#pragma once

//Define this before including to disable stb_image usage
//#define VOLUMARCHER_NO_STB_IMAGE

//Define this before including to disable background environment example
//#define VOLUMARCHER_NO_SKYBOX

#include "BlueNoise.h"
#include "DetailNoise.h"
#include "../MiniEngine/Core/CommandContext.h"
#include "../MiniEngine/Core/RootSignature.h"
#include "../MiniEngine/Core/PipelineState.h"
#include "../MiniEngine/Core/BufferManager.h"
#include "../MiniEngine/Core/UploadBuffer.h"


#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "VoxelWorld.h"
#include "../shaders/ShaderCommon.h"

namespace openvdb::v12_0::io
{
	class File;
}

namespace Volumarcher
{
	struct CameraSettings
	{
		//zNear of _inputDepth
		float zNear;
		//zFar of _inputDepth
		float zFar;
		//Vertical Fov in degrees
		float vFov;
	};

	struct QualitySettings
	{
		//Resolution of internal volumetric outputBuffer, scales up to screenRes when compositing
		struct Resolution
		{
			int x = 1920;
			int y = 1080;
		} cloudResolution;

		//Amount of reprojection applied
		float reprojectAmount = 1.f;
		int baseSampleCount = 128;
		//Real light samples to take per sample, this is very expensive
		int realDirectLightingSampleCount = 2;
		//Cached light samples, considerably cheaper than real samples but less accurate
		int cachedDirectLightingSampleCount = 8;
		//Updates a chunk of N*N*N voxels per frame for cached lighting
		int lightingCacheUpdateChunkSize = 64;
		int ambientSampleCount = 32;

		//Size of the lighting cache voxels compared to actual data, lower values means more accurate lighting for more cost
		unsigned int lightCacheVoxelSize = 3u;

		// Target distance in abstract units to render at a high quality, for when close to a cloud / going through it
		float highQualityRenderDistance = 30;
		// Target distance in abstract units to render clouds at lowering quality over distance, for when looking at distance clouds
		float lowQualityRenderDistance = 70;

		// Target distance in abstract units to render direct light shadows
		float targetLightRenderDistance = 60;
	};

	struct EnvironmentSettings
	{
		glm::vec3 sunDirection = glm::normalize(glm::vec3(0.5f, -1.f, 0.5f));
		glm::vec3 sunLight = (glm::vec3(0.996f, 0.6f, 0.8f) * 4.f);
		//Light coming from the environment/skybox this should be an integrated upper hemisphere
		glm::vec3 ambientLight = (glm::vec3(3.14f));
	};

	struct ArtisticSettings
	{
		float densityScale = 1.f;
		// Directionality of light strength, Causes silver lining effect
		float eccentricity = 0.3f;
		glm::vec3 cloudColor = glm::vec3(1.f);
		//Moves and tiles the clouds around inside their bounds, useful for adding wind
		glm::vec2 cloudOffset = glm::vec3(0.f);
		//wind amount to move the detail noise
		glm::vec3 noiseWind = glm::vec3(1.f, 0.f, 1.f) * 0.1f;
		float noiseFrequency = 3.f; // Frequency of the high detail noise
	};

	enum class Result : std::uint8_t
	{
		Succeeded,
		IoError,
		Failed,
	};

	struct PointLight
	{
		glm::vec3 position;
		float radius;
		glm::vec3 light;
		float padding;
	};

	class VolumarcherContext
	{
	public:
		VolumarcherContext() = delete;

		/// <summary>
		/// Creates renderingcontext for Volumarcher
		/// </summary>
		/// <param name="_detailNoise">3D noise used for adding detail, use the libraries supplied textures if unsure</param>
		/// <param name="_blueNoise">Blue noise to use for screen jittering, use library supplied textures</param>
		/// <param name="_cameraSettings">Settings of the camera used in your rasterization pass</param>
		/// <param name="_qualitySettings"></param>
		/// <param name="_lookSettings"></param>
		explicit VolumarcherContext(const DetailNoise& _detailNoise, const BlueNoise& _blueNoise,
		                            const CameraSettings& _cameraSettings,
		                            const QualitySettings& _qualitySettings = {},
		                            const ArtisticSettings& _lookSettings = {});


		/// <summary>
		/// Load a VDB grid into the world to be rendered
		/// </summary>
		/// <param name="_vdb">Path to the file\n VDB should contain at least:\nA
		/// A grid with density profile (0-1) called "dimensional_profile" or "density"\n
		/// A grid with a SDF in voxel space towards where profile > 0, called "sdf" or "distance"</param>
		/// <param name="_gridScale">scales 1 voxel to be this size</param>
		/// <param name="_location">Center position where to put the grid</param>
		/// <param name="_densityScale">Scales final uprezed density</param>
		/// <param name="_sdfMaxValue">Clamps values in the SDF(if any) if using the provided houdini asset this is 10 (halfband size)</param>
		[[nodiscard]] Result LoadGrid(const std::string& _vdb, float _gridScale = 1.f,
		                              glm::vec3 _location = glm::vec3(0.f),
		                              float _sdfMaxValue = 10.f);

		/// <summary>
		/// Load a VDB grid into the world to be rendered
		/// </summary>
		/// <param name="_vdb">VDB File loaded with openvdb\n VDB should contain at least:\nA
		/// A grid with density profile (0-1) called "dimensional_profile" or "density"\n
		/// A grid with a SDF in voxel space towards where profile > 0, called "sdf" or "distance"</param>
		/// <param name="_gridScale">scales 1 voxel to be this size</param>
		/// <param name="_location">Center position where to put the grid</param>
		/// <param name="_densityScale">Scales final uprezed density</param>
		/// <param name="_sdfMaxValue">Clamps values in the SDF(if any) if using the provided houdini asset this is 10 (halfband size)</param>
		[[nodiscard]] Result LoadGrid(openvdb::io::File& _vdb, float _gridScale = 1.f,
		                              glm::vec3 _location = glm::vec3(0.f),
		                              float _sdfMaxValue = 10.f);


		[[nodiscard]] const QualitySettings& GetQualitySettings() const { return m_settings; }

		///Updates settings, depending on what changed it has to recalculate cached data 
		void SetQualitySettingsAndUpdateGrid(const QualitySettings& _volumetricSettings);

		[[nodiscard]] const EnvironmentSettings& GetEnvironmentSettings() const { return m_environmentSettings; }
		void SetEnvironmentSettings(const EnvironmentSettings& _settings) { m_environmentSettings = _settings; }

		[[nodiscard]] const ArtisticSettings& GetCloudLookSettings() const { return m_cloudLookSettings; }

		void SetCloudLookSettings(const ArtisticSettings& _settings);

		void Update(float _deltaTime);

		/// <summary>
		/// Renders the volumetrics. First load a grid with LoadGrid
		/// </summary>
		/// <param name="_outputBuffer">Float4 Color buffer to output in</param>
		/// <param name="_outputBufferState">State to return the buffer to</param>
		/// <param name="_inputDepth">Input depthbuffer to use for merging with first pass</param>
		/// <param name="_camPos">Current position of the camera</param>
		/// <param name="_camForward">Current rotation of the camera</param>
		void RenderVolumetrics(ColorBuffer _outputBuffer, D3D12_RESOURCE_STATES _outputBufferState,
		                       DepthBuffer _inputDepth,
		                       glm::vec3 _camPos,
		                       glm::vec3 _camForward);



		/// <summary>
		///Render a realistic generated atmosphere skybox based on sunDirection
		/// </summary>
		/// <param name="_outputBuffer">Float4 Color buffer to output in</param>
		/// <param name="_outputBufferState">State to return the buffer to</param>
		/// <param name="_inputDepth">Input depthbuffer to use for merging with first pass</param>
		/// <param name="_camPos">Current position of the camera</param>
		/// <param name="_camForward">Current rotation of the camera</param>
		void RenderSkyBackground(ColorBuffer _outputBuffer, D3D12_RESOURCE_STATES _outputBufferState,
		                         DepthBuffer _inputDepth,
		                         glm::vec3 _camForward) const;
#ifndef VOLUMARCHER_NO_SKYBOX
		glm::vec3 GetSkyBackgroundAmbient(glm::vec3 _sunDir);

		glm::vec3 GetSkyBackgroundSunlight(glm::vec3 _sunDir);

#endif

		void SetPointLights(std::vector<PointLight> _lights);

	private:
		friend class CloudShadowMap;

		void UpdateShaderConstants();

		void UpdateDirectLighting();

		//TODO: Make this user specified 
		Internal::VoxelWorld m_grid;
		DetailNoise m_noise;
		BlueNoise m_blueNoise;
		ComputePSO m_computePSO{L"Volumarcher: Render PSO"};
		ComputePSO m_blitPSO{L"Volumarcher: blitting PSO"};
#ifndef VOLUMARCHER_NO_SKYBOX
		ComputePSO m_skyPSO{L"Volumarcher: Skybox rendering PSO"};
#endif
		ComputePSO m_directLightPSO{L"Volumarcher: Lighting update PSO"};
		RootSignature m_rs;
		RootSignature m_blitRs;
#ifndef VOLUMARCHER_NO_SKYBOX
		RootSignature m_skyRs;
#endif

		ColorBuffer m_renderBuffer;
		RootSignature m_lightRs;
		CameraSettings m_cameraSettings;
		QualitySettings m_settings;
		EnvironmentSettings m_environmentSettings;
		ArtisticSettings m_cloudLookSettings;
		VolumetricSettings m_volumetricConstants;
		UploadBuffer m_volumetricConstantsBuffer;

		StructuredBuffer m_lightBuffer;
		UploadBuffer m_lightBufferUpload;

		glm::vec3 m_densityGridOrigin{0.f};
		glm::vec3 m_densityGridScale{0.f};
		glm::vec3 m_distanceGridOrigin{0.f};
		glm::vec3 m_distanceGridScale{0.f};
		glm::vec3 m_oldCamPos;
		glm::vec3 m_oldCamDir;

		glm::ivec2 m_outputRes;

		double m_time{0.f};
		float m_sdfScale{0.f};
		float m_gridScale{0.f};
		float m_sdfMaxValue{10.f};
		float m_deltaTime{0.f};
		unsigned int m_frame{0};
		unsigned int m_activeLights{0};
		bool m_renderBufferCleared{false};
		bool m_lightsUpdated{false};
	};
}
