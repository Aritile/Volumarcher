#include "Volumarcher.h"
#define IMATH_HALF_NO_LOOKUP_TABLE
#pragma warning(push,0)
#include <openvdb/openvdb.h>
#pragma warning(pop)

#include "../MiniEngine/Core/CommandContext.h"
#include "../MiniEngine/Core/RootSignature.h"
#include "../MiniEngine/Core/PipelineState.h"

#include "../shaders/ShaderCommon.h"

#include "glm/trigonometric.hpp"

namespace Volumarcher
{
#include "CompiledShaders/VolumetricsCS.h"
#include "CompiledShaders/CalculateDirectLighting.h"
#include "CompiledShaders/AtmosphereCS.h"
#include "CompiledShaders/CopyToOutput.h"

	enum ComputeRootsigRegisters : uint8_t
	{
		OutputTexture,
		SceneDepth,
		Lights,
		Dynamics,
		Settings,
		World,
		Voxels,
		END
	};

	VolumarcherContext::VolumarcherContext(const DetailNoise& _detailNoise, const BlueNoise& _blueNoise,
	                                       const CameraSettings& _cameraSettings,
	                                       const QualitySettings& _qualitySettings,
	                                       const ArtisticSettings& _lookSettings) :
		m_grid(),
		m_noise(_detailNoise),
		m_blueNoise(_blueNoise),
		m_cameraSettings(_cameraSettings),
		m_settings(_qualitySettings),
		m_cloudLookSettings(_lookSettings)
	{
		//Linear wrap sampler
		D3D12_SAMPLER_DESC noiseSamplerDesc{
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
			D3D12_TEXTURE_ADDRESS_MODE_MIRROR, D3D12_TEXTURE_ADDRESS_MODE_MIRROR, 0, 0, D3D12_COMPARISON_FUNC_NONE,
			{0, 0, 0, 1}, 0, 0
		};
		//Linear clamp sampler
		D3D12_SAMPLER_DESC profileSamplerDesc{
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 0, D3D12_COMPARISON_FUNC_NONE,
			{0, 0, 0, 1}, 0, 0
		};

		//point clamp sampler
		D3D12_SAMPLER_DESC pointSamplerDesc{
			D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, 0, 0, D3D12_COMPARISON_FUNC_NONE,
			{0, 0, 0, 1}, 0, 0
		};

		D3D12_SAMPLER_DESC bluenoiseSamplerDesc{
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 0, D3D12_COMPARISON_FUNC_NONE,
			{0, 0, 0, 1}, 0, 0
		};

		//PSO for main render
		{
			m_rs.Reset(ComputeRootsigRegisters::END, 4);
			//Buffers
			m_rs[ComputeRootsigRegisters::OutputTexture].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
			m_rs[ComputeRootsigRegisters::SceneDepth].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
			m_rs[ComputeRootsigRegisters::Lights].InitAsBufferSRV(7);

			//Settings
			m_rs[ComputeRootsigRegisters::Dynamics].InitAsConstants(0, sizeof(VolumetricDynamics) / sizeof(uint32_t));
			m_rs[ComputeRootsigRegisters::Settings].InitAsConstantBuffer(1);
			m_rs[ComputeRootsigRegisters::World].InitAsConstants(2, sizeof(VolumetricWorld) / sizeof(uint32_t));

			//Voxels: lightCache, Noise, Density, Distance, bluenoise, color
			m_rs[ComputeRootsigRegisters::Voxels].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);

			m_rs.InitStaticSampler(0, noiseSamplerDesc);
			m_rs.InitStaticSampler(1, profileSamplerDesc);
			m_rs.InitStaticSampler(2, bluenoiseSamplerDesc);
			m_rs.InitStaticSampler(3, pointSamplerDesc);

			m_rs.Finalize(L"Volumarcher: Render RS");
			m_computePSO.SetRootSignature(m_rs);
			m_computePSO.SetComputeShader(g_pVolumetricsCS, sizeof(g_pVolumetricsCS));
			m_computePSO.Finalize();
		}

		//PSO for blitting
		{
			m_blitRs.Reset(3, 1);
			//Buffers
			m_blitRs[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1); // Output
			m_blitRs[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1); // History buffer

			//Settings
			m_blitRs[2].InitAsConstants(0, sizeof(BlitValues) / sizeof(uint32_t));

			m_blitRs.InitStaticSampler(0, profileSamplerDesc);

			m_blitRs.Finalize(L"Volumarcher: Blit RS");

			m_blitPSO.SetRootSignature(m_blitRs);
			m_blitPSO.SetComputeShader(g_pCopyToOutput, sizeof(g_pCopyToOutput));
			m_blitPSO.Finalize();
		}

		{
			//PSO for lighting update
			m_lightRs.Reset(5, 2);
			//Output cache texture
			m_lightRs[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
			//constants
			m_lightRs[1].InitAsConstants(0, sizeof(DirectLightingSettings) / sizeof(uint32_t));

			//Profile texture
			m_lightRs[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
			//SDF Texture
			m_lightRs[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
			//Noise textures
			m_lightRs[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1);

			m_lightRs.InitStaticSampler(0, noiseSamplerDesc);
			m_lightRs.InitStaticSampler(1, profileSamplerDesc);

			m_lightRs.Finalize(L"Volumarcher: DirectLight update RS");
			m_directLightPSO.SetRootSignature(m_lightRs);
			m_directLightPSO.SetComputeShader(g_pCalculateDirectLighting, sizeof(g_pCalculateDirectLighting));
			m_directLightPSO.Finalize();
		}

#ifndef VOLUMARCHER_NO_SKYBOX
		//PSO for skybox
		{
			//PSO for lighting update
			m_skyRs.Reset(3, 1);
			//Output cache texture
			m_skyRs[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
			//constants
			m_skyRs[1].InitAsConstants(0, sizeof(AtmosphereRenderConstants) / sizeof(uint32_t));
			//depth input
			m_skyRs[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);

			m_skyRs.InitStaticSampler(0, profileSamplerDesc);

			m_skyRs.Finalize(L"Volumarcher: Skybox rendering RS");
			m_skyPSO.SetRootSignature(m_skyRs);
			m_skyPSO.SetComputeShader(g_pAtmosphereCS, sizeof(g_pAtmosphereCS));
			m_skyPSO.Finalize();
		}
#endif

		m_volumetricConstantsBuffer.Create(L"Quality Settings", sizeof(VolumetricSettings));
		*static_cast<VolumetricSettings*>(m_volumetricConstantsBuffer.Map()) = m_volumetricConstants;

		m_renderBuffer.Create(L"Volumarcher History buffer", m_settings.cloudResolution.x,
		                      m_settings.cloudResolution.y,
		                      1,
		                      DXGI_FORMAT_R16G16B16A16_FLOAT);
		m_renderBuffer.SetClearColor({0.f, 0.f, 0.f, 0.f});

		m_lightBufferUpload.Create(L"Light upload buffer", sizeof(LightSource) * MAX_LIGHTSOURCES);
		static_cast<LightSource*>(m_lightBufferUpload.Map())[0] = LightSource{
			glm::vec3(0, 0, 0), 0.f, glm::vec3(0, 0, 0), 0.f
		};
		m_lightBuffer.Create(L"Lights buffer", MAX_LIGHTSOURCES, sizeof(LightSource), m_lightBufferUpload);
	}

	Result VolumarcherContext::LoadGrid(openvdb::io::File& _vdb, const float _gridScale,
	                                    const glm::vec3 _location,
	                                    const float _sdfMaxValue)
	{
		auto result = m_grid.LoadVDB(_vdb, _sdfMaxValue, m_settings.lightCacheVoxelSize, m_settings.ambientSampleCount);
		if (result == Result::Succeeded)
		{
			m_grid.SetDensityScale(m_cloudLookSettings.densityScale);
			glm::vec3 densityGridExtend = (glm::vec3(m_grid.GetDensityGridSize()) * _gridScale);
			glm::vec3 distanceGridExtend = (glm::vec3(m_grid.GetDistanceGridSize()) * _gridScale);
			m_densityGridScale = 1.f / densityGridExtend;
			m_distanceGridScale = 1.f / distanceGridExtend;

			m_gridScale = _gridScale;
			m_sdfScale = _gridScale;
			m_sdfMaxValue = _sdfMaxValue;
			//_origin -= densityGridExtend * 0.5f;
			m_densityGridOrigin = _location + glm::vec3(m_grid.GetDensityGridOrigin()) * _gridScale;
			m_distanceGridOrigin = _location + glm::vec3(m_grid.GetDistanceGridOrigin()) * _gridScale;
		}
		return result;
	}

	Result VolumarcherContext::LoadGrid(const std::string& _vdb, const float _gridScale, const glm::vec3 _location,
	                                    const float _sdfMaxValue)

	{
		openvdb::initialize();

		openvdb::io::File file(_vdb);
		try
		{
			file.open();
		}
		catch (const openvdb::IoError& e)
		{
			Utility::Printf((std::string("Volumarcher Error: ") + e.what() + '\n').c_str());
			return Result::IoError;
		}
		catch (const std::exception& e)
		{
			Utility::Printf((std::string("Volumarcher Unknown error: ") + e.what() + '\n').c_str());
			return Result::Failed;
		}
		auto res = LoadGrid(file, _gridScale, _location, _sdfMaxValue);
		file.close();
		return res;
	}

	void VolumarcherContext::SetQualitySettingsAndUpdateGrid(const QualitySettings& _volumetricSettings)
	{
		bool updateCache = m_settings.ambientSampleCount != _volumetricSettings.ambientSampleCount;
		bool recreateCache = m_settings.lightCacheVoxelSize != _volumetricSettings.lightCacheVoxelSize;
		bool recreateBuffer = m_settings.cloudResolution.x != _volumetricSettings.cloudResolution.x
			|| m_settings.cloudResolution.y != _volumetricSettings.cloudResolution.y;

		m_settings = _volumetricSettings;
		m_settings.reprojectAmount = std::min(std::max(_volumetricSettings.reprojectAmount, 0.f), 1.f);
		UpdateShaderConstants();
		if (recreateCache)
		{
			//Recreate cache
			m_grid.RecreateLightCache(_volumetricSettings.lightCacheVoxelSize);
		}
		if (updateCache && !recreateCache)
		{
			Utility::Print("Volumarcher: Recalculating cached data");
			m_grid.RecalculateCachedLighting(m_settings.ambientSampleCount);
		}
		if (recreateBuffer)
		{
			auto& fenceContext = ComputeContext::Begin(L"Volumarcher: Sync history buffer");
			fenceContext.TransitionResource(m_renderBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
			fenceContext.Finish(true);
			m_renderBuffer.Destroy();
			m_renderBuffer.Create(L"Volumarcher History buffer", m_settings.cloudResolution.x,
			                      m_settings.cloudResolution.y,
			                      1,
			                      DXGI_FORMAT_R16G16B16A16_FLOAT);
			m_renderBuffer.SetClearColor({0.f, 0.f, 0.f, 0.f});

			m_renderBufferCleared = false;
		}
	}


	void VolumarcherContext::SetCloudLookSettings(const ArtisticSettings& _settings)
	{
		m_grid.SetDensityScale(_settings.densityScale);
		m_cloudLookSettings = _settings;
		m_cloudLookSettings.eccentricity = std::min(_settings.eccentricity, 0.99f);
	}

	void VolumarcherContext::Update(const float _deltaTime)
	{
		m_time += _deltaTime;
		m_deltaTime = _deltaTime;
		m_frame++;
	}


	void VolumarcherContext::RenderVolumetrics(ColorBuffer _outputBuffer, D3D12_RESOURCE_STATES _outputBufferState,
	                                           DepthBuffer _inputDepth, glm::vec3 _camPos,
	                                           glm::vec3 _camForward)
	{
		if (!m_grid.IsLoaded())
		{
			Utility::Printf("ERROR: Volumarcher has no grid loaded, use LoadGrid() before rendering\n");
			assert(false);
			return;
		}

		unsigned int outResX = _outputBuffer.GetWidth();
		unsigned int outResY = _outputBuffer.GetHeight();
		if (outResX != static_cast<unsigned int>(m_outputRes.x) || outResY != static_cast<unsigned int>(m_outputRes.y))
		{
			m_outputRes.x = outResX;
			m_outputRes.y = outResY;
			UpdateShaderConstants();
		}


		//Update lighting cache
		if (m_settings.cachedDirectLightingSampleCount > 0) UpdateDirectLighting();

		//Render volumetrics into buffer
		{
			ComputeContext& computeContext = ComputeContext::Begin(L"Volumarcher: Render volumetrics");
			computeContext.SetPipelineState(m_computePSO);
			computeContext.SetRootSignature(m_rs);
			glm::vec3 camDir = _camForward;

			if (!m_renderBufferCleared)
			{
				m_renderBufferCleared = true;
				m_oldCamPos = _camPos;
				m_oldCamDir = camDir;
				computeContext.ClearUAV(m_renderBuffer);
			}

			//Bind output
			computeContext.TransitionResource(m_renderBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			computeContext.SetDynamicDescriptor(ComputeRootsigRegisters::OutputTexture, 0, m_renderBuffer.GetUAV());
			computeContext.SetDynamicDescriptor(ComputeRootsigRegisters::SceneDepth, 0, _inputDepth.GetDepthSRV());
			//Bind variables
			auto screenX = m_settings.cloudResolution.x;
			auto screenY = m_settings.cloudResolution.y;

			VolumetricDynamics cameraSettings{
				_camPos, static_cast<float>(m_time), camDir, 0.f, m_oldCamPos,
				static_cast<int>(m_activeLights),
				m_oldCamDir, m_deltaTime
			};

			m_oldCamPos = _camPos;
			m_oldCamDir = camDir;

			VolumetricWorld worldSettings{
				m_cloudLookSettings.noiseWind,
				m_grid.GetDensityScale(),
				m_environmentSettings.sunDirection,
				m_cloudLookSettings.eccentricity,
				m_environmentSettings.sunLight,
				m_cloudLookSettings.cloudOffset.x,
				m_cloudLookSettings.cloudColor,
				m_cloudLookSettings.cloudOffset.y,
				m_environmentSettings.ambientLight
			};

			computeContext.SetConstantArray(ComputeRootsigRegisters::Dynamics,
			                                sizeof(VolumetricDynamics) / sizeof(uint32_t), &cameraSettings);
			computeContext.SetConstantBuffer(ComputeRootsigRegisters::Settings,
			                                 m_volumetricConstantsBuffer.GetGpuVirtualAddress());
			computeContext.SetConstantArray(ComputeRootsigRegisters::World, sizeof(VolumetricWorld) / sizeof(uint32_t),
			                                &worldSettings);

			if (m_lightsUpdated)
			{
				m_lightsUpdated = false;
				computeContext.TransitionResource(m_lightBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				computeContext.CopyBuffer(m_lightBuffer, m_lightBufferUpload);
			}
			computeContext.TransitionResource(m_lightBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			computeContext.SetBufferSRV(ComputeRootsigRegisters::Lights, m_lightBuffer);

			//Cache data
			computeContext.TransitionResource(m_grid.GetLightCache(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			computeContext.SetDynamicDescriptor(ComputeRootsigRegisters::Voxels, 0, m_grid.GetLightCache().GetSRV());
			computeContext.SetDynamicDescriptor(ComputeRootsigRegisters::Voxels, 1, m_noise.GetNoise().GetSRV());
			computeContext.SetDynamicDescriptor(ComputeRootsigRegisters::Voxels, 2, m_grid.GetDensityField().GetSRV());
			if (m_grid.HasDistanceField())
				computeContext.SetDynamicDescriptor(ComputeRootsigRegisters::Voxels, 3,
				                                    m_grid.GetDistanceField().GetSRV());
			if (m_grid.HasColorDensityScaleField())
				computeContext.SetDynamicDescriptor(ComputeRootsigRegisters::Voxels, 5,
				                                    m_grid.GetColorDensityScaleField().GetSRV());

			computeContext.SetDynamicDescriptor(ComputeRootsigRegisters::Voxels, 4,
			                                    m_blueNoise.GetNoise(static_cast<float>(m_time * 20.f)).GetSRV());

			//End call
			static constexpr int GROUP_SIZE = VOLUMETRIC_PASS_GROUP_SIZE;
			computeContext.Dispatch2D(screenX, screenY, GROUP_SIZE, GROUP_SIZE);


			computeContext.Finish();
		}

		// To output
		{
			ComputeContext& computeContext = ComputeContext::Begin(L"Volumarcher: Blit to screen");

			computeContext.SetPipelineState(m_blitPSO);
			computeContext.SetRootSignature(m_blitRs);

			computeContext.TransitionResource(_outputBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			computeContext.TransitionResource(m_renderBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			computeContext.SetDynamicDescriptor(0, 0, _outputBuffer.GetUAV());
			computeContext.SetDynamicDescriptor(1, 0, m_renderBuffer.GetSRV());

			BlitValues constants{
				outResX,
				outResY
			};

			computeContext.SetConstantArray(2, sizeof(constants) / sizeof(uint32_t), &constants);
			static constexpr int GROUP_SIZE = COPY_GROUP_SIZE;
			computeContext.Dispatch2D(outResX, outResY, GROUP_SIZE, GROUP_SIZE);

			computeContext.TransitionResource(_outputBuffer, _outputBufferState);

			computeContext.Finish();
		}
	}

	void VolumarcherContext::RenderSkyBackground(ColorBuffer _outputBuffer, D3D12_RESOURCE_STATES _outputBufferState,
	                                             DepthBuffer _inputDepth, glm::vec3 _camForward) const
	{
#ifndef VOLUMARCHER_NO_SKYBOX
		ComputeContext& computeContext = ComputeContext::Begin(L"Volumarcher: Render background sky");
		computeContext.SetPipelineState(m_skyPSO);
		computeContext.SetRootSignature(m_skyRs);

		//Bind output
		computeContext.TransitionResource(_outputBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
		computeContext.SetDynamicDescriptor(0, 0, _outputBuffer.GetUAV());
		computeContext.SetDynamicDescriptor(2, 0, _inputDepth.GetDepthSRV());
		//Bind variables
		auto screenX = _outputBuffer.GetWidth();
		auto screenY = _outputBuffer.GetHeight();


		AtmosphereRenderConstants constants{
			_camForward,
			tan(glm::radians(m_cameraSettings.vFov) / 2.f),
			m_environmentSettings.sunDirection,
			1.f / static_cast<float>(screenX),
			1.f / static_cast<float>(screenY),
			screenX / static_cast<float>(screenY),
			0.f,
			0.f
		};

		computeContext.SetConstantArray(1, sizeof(AtmosphereRenderConstants) / sizeof(uint32_t), &constants);

		//End call
		static constexpr int GROUP_SIZE = 16;
		computeContext.Dispatch2D(screenX, screenY, GROUP_SIZE, GROUP_SIZE);

		computeContext.TransitionResource(_outputBuffer, _outputBufferState, false);

		computeContext.Finish();
#else
		Utility::Printf("Volumarcher: WARNING, Trying to use skybox functions while VOLUMARCHER_NO_SKYBOX is defined\n")
#endif
	}
#ifndef VOLUMARCHER_NO_SKYBOX

#define zenithDensity(x) 0.7f / pow(std::max(x, 0.35e-2f), 0.75f)

	glm::vec3 getSkyAbsorption(glm::vec3 x, float y)
	{
		glm::vec3 absorption = x * -y;
		absorption = exp2(absorption) * 2.0f;

		return absorption;
	}

	float getRayleigMultiplier(glm::vec3 rayDir, glm::vec3 sunDir)
	{
		float cosAngle = std::clamp(dot(rayDir, sunDir), 0.f, 1.f);
		return 1.0f + pow(1.0f - cosAngle, 2.0f) * 3.14159f * 0.5f;
	}

	glm::vec3 VolumarcherContext::GetSkyBackgroundAmbient(glm::vec3 _sunDir)
	{
		//Approximated instead of integrated but good enough

		static const float multiScatterPhase = 0.1;
		static const glm::vec3 skyColorDay = glm::vec3(0.39f, 0.57f, 1.0f);
		static const glm::vec3 skyColorNight = glm::vec3(0.98f, 0.98f, 0.8f);


		glm::vec3 sunDir = _sunDir;
		glm::vec3 rayDir = glm::normalize(glm::vec3(0.5f, 0.7f, 0.5f));

		float night = 1 - std::clamp((sunDir.y + 0.1f) * 7.f, 0.f, 1.f);

		float zenith = zenithDensity(rayDir.y + (0.1f * night));
		float sunPointDistMult = std::clamp(glm::length(std::max(sunDir.y + multiScatterPhase, 0.0f)), 0.0f, 1.0f);

		glm::vec3 skyColor = glm::mix(skyColorDay, skyColorNight, night);

		float rayleighMult = getRayleigMultiplier(rayDir, -sunDir);

		glm::vec3 absorption = getSkyAbsorption(skyColor, zenith);
		glm::vec3 sunAbsorption = getSkyAbsorption(skyColor, zenithDensity(sunDir.y + multiScatterPhase));
		glm::vec3 sky = skyColor * zenith * rayleighMult;

		glm::vec3 totalSky = mix(sky * absorption, sky / (sky + 0.5f), sunPointDistMult);
		totalSky *= sunAbsorption * 0.5f + 0.5f * length(sunAbsorption);

		//Night
		totalSky = mix(totalSky, absorption * 0.2f, night);

		return totalSky * 0.05f;
	}


	glm::vec3 VolumarcherContext::GetSkyBackgroundSunlight(glm::vec3 _sunDir)
	{
		float night = 1 - std::clamp((_sunDir.y + 0.1f) * 7, 0.f, 1.f);
		float zenith = zenithDensity(_sunDir.y + (0.1f * night));
		static const glm::vec3 skyColorDay = glm::vec3(0.39f, 0.57f, 1.0f);
		static const glm::vec3 skyColorNight = glm::vec3(0.98f, 0.98f, 0.8f);

		glm::vec3 skyColor = glm::mix(skyColorDay, skyColorNight, night);

		glm::vec3 sun = getSkyAbsorption(skyColor, zenith) * 5.f;
		return sun;
	}
#endif

	void VolumarcherContext::SetPointLights(std::vector<PointLight> _lights)
	{
		auto* buffer = static_cast<LightSource*>(m_lightBufferUpload.Map());
		const unsigned int amount = std::min(static_cast<unsigned int>(_lights.size()), MAX_LIGHTSOURCES);
		m_activeLights = amount;
		for (unsigned int i = 0; i < amount; ++i)
		{
			buffer[i] = *reinterpret_cast<LightSource*>(&_lights[i]);
		}
		m_lightsUpdated = true;
	}

	void VolumarcherContext::UpdateShaderConstants()
	{
		//helpers to make settings more intuitive
		float targetRenderDistance = m_settings.highQualityRenderDistance;
		static constexpr float ADAPTIVE_SCALE = 0.00025f;
		float adaptiveStepDistance = (m_settings.lowQualityRenderDistance * m_settings.
			lowQualityRenderDistance) * ADAPTIVE_SCALE;


		m_volumetricConstants = VolumetricSettings{
			m_densityGridOrigin, m_settings.baseSampleCount,
			m_densityGridScale,
			m_settings.realDirectLightingSampleCount + m_settings.cachedDirectLightingSampleCount,
			m_distanceGridOrigin,
			targetRenderDistance,
			m_distanceGridScale,
			static_cast<uint>(m_settings.cloudResolution.x), static_cast<uint>(m_settings.cloudResolution.y),
			m_cameraSettings.zNear,
			m_cameraSettings.zFar,
			tan(glm::radians(m_cameraSettings.vFov) / 2.f),
			m_grid.HasDistanceField() ? m_sdfScale : 0.f,
			m_gridScale * m_cloudLookSettings.noiseFrequency,
			m_gridScale,
			m_sdfMaxValue,
			m_settings.realDirectLightingSampleCount,
			(m_settings.cachedDirectLightingSampleCount > 0)
				? m_settings.targetLightRenderDistance * 0.1f
				: m_settings.targetLightRenderDistance,
			glm::radians(m_cameraSettings.vFov),
			adaptiveStepDistance,
			(m_outputRes.x / static_cast<float>(m_settings.cloudResolution.x)),
			m_outputRes.x / static_cast<float>(m_outputRes.y),
			m_settings.reprojectAmount,
			m_grid.HasColorDensityScaleField() ? 1 : 0
		};
		*static_cast<VolumetricSettings*>(m_volumetricConstantsBuffer.Map()) = m_volumetricConstants;
	}

	void VolumarcherContext::UpdateDirectLighting()
	{
		ComputeContext& computeContext = ComputeContext::Begin(L"Volumarcher: Lighting update");
		computeContext.SetPipelineState(m_directLightPSO);
		computeContext.SetRootSignature(m_lightRs);

		//Bind output
		computeContext.TransitionResource(m_grid.GetLightCache(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.SetDynamicDescriptor(0, 0, m_grid.GetLightCacheUAV());

		//Input
		computeContext.SetDynamicDescriptor(2, 0, m_grid.GetDensityField().GetSRV());
		if (m_grid.HasDistanceField())
			computeContext.SetDynamicDescriptor(3, 0, m_grid.GetDistanceField().GetSRV());
		computeContext.SetDynamicDescriptor(4, 0, m_noise.GetNoise().GetSRV());
		//Bind variables
		auto outX = m_grid.GetLightCache().GetWidth();
		auto outY = m_grid.GetLightCache().GetHeight();
		auto outZ = m_grid.GetLightCache().GetDepth();

		//Dispatch in chunks to spread load over frames
		int3 dispatchOffset = int3{
			0
		};

		int3 dispatchSize = {
			outX, outY, outZ
		};
		if (m_settings.lightingCacheUpdateChunkSize > 0)
		{
			//Size to dispatch per render, higher means calculate more of the grid per frame
			int3 maxDispatchSize = int3{m_settings.lightingCacheUpdateChunkSize};

			int3 chunkCount = {
				(outX + maxDispatchSize.x - 1) / maxDispatchSize.x,
				(outY + maxDispatchSize.y - 1) / maxDispatchSize.y,
				(outZ + maxDispatchSize.z - 1) / maxDispatchSize.z
			};

			int totalChunks =
				chunkCount.x * chunkCount.y * chunkCount.z;

			unsigned int frameIndex = (m_frame) % totalChunks;

			int cZ = frameIndex / (chunkCount.x * chunkCount.y);
			int rem = frameIndex % (chunkCount.x * chunkCount.y);
			int cY = rem / chunkCount.x;
			int cX = rem % chunkCount.x;

			dispatchOffset = {
				cX * maxDispatchSize.x,
				cY * maxDispatchSize.y,
				cZ * maxDispatchSize.z
			};

			dispatchSize = {
				std::min(static_cast<unsigned int>(maxDispatchSize.x),
				         outX - static_cast<unsigned int>(dispatchOffset.x)),
				std::min(static_cast<unsigned int>(maxDispatchSize.y),
				         outY - static_cast<unsigned int>(dispatchOffset.y)),
				std::min(static_cast<unsigned int>(maxDispatchSize.z),
				         outZ - static_cast<unsigned int>(dispatchOffset.z))
			};
		}

		DirectLightingSettings constants{
			int3(outX, outY, outZ),
			1 / static_cast<float>(m_grid.GetCacheVoxelSize()),
			m_environmentSettings.sunDirection,
			m_settings.cachedDirectLightingSampleCount, m_cloudLookSettings.noiseWind, static_cast<float>(m_time),
			dispatchOffset,
			m_cloudLookSettings.noiseFrequency, m_settings.targetLightRenderDistance
		};

		computeContext.SetConstantArray(1, sizeof(DirectLightingSettings) / sizeof(uint32_t), &constants);
		//End call
		static constexpr int GROUP_SIZE = 8;
		computeContext.Dispatch3D(dispatchSize.x, dispatchSize.y, dispatchSize.z, GROUP_SIZE, GROUP_SIZE, GROUP_SIZE);

		computeContext.Finish();
	}
}
