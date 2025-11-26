#pragma once
#include "../MiniEngine/Core/CommandContext.h"
#include "../MiniEngine/Core/RootSignature.h"
#include "../MiniEngine/Core/PipelineState.h"
#include "../MiniEngine/Core/BufferManager.h"
#include "../shaders/ShaderCommon.h"


#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "CloudNoise.h"
#include "ImageNoise.h"

namespace Volumarcher
{
	struct CameraSettings
	{
		//zNear of _inputDepth
		float zNear;
		//zFar of _inputDepth
		float zFar;
		//Vertical Fov in degrees
		float vFov; //TODO: Use
	};

	struct Settings
	{
		int baseSampleCount = 128;
		int lightingSampleCount = 10;
		int ambientSampleCount = 3;
	};

	class VolumetricContext
	{
	public:
		VolumetricContext() = delete;

		explicit VolumetricContext(Volume _volumes[VOLUME_AMOUNT], CameraSettings _cameraSettings,
		                           Settings _settings = {});


		//void SetVolumes(Volume _volumes[VOLUME_AMOUNT]);

		void SetVolumeGrid(std::vector<float> _densityGrid, glm::ivec3 _size, glm::vec3 _worldSpaceSize);

		Settings GetSettings() const { return m_settings; }
		void SetSettings(const Settings _volumetricSettings) { m_settings = _volumetricSettings; }

		void Update(float _deltaTime);

		void Render(ColorBuffer _outputBuffer, D3D12_RESOURCE_STATES _outputBufferState, DepthBuffer _inputDepth,
		            glm::vec3 _camPos = glm::vec3{0.f},
		            glm::quat _camRot = glm::identity<glm::quat>());

	private:
		//TODO: Make this user specified 
		ImageNoise m_noise;

		Texture m_cloudVolumeVoxels;

		ComputePSO m_computePSO;
		RootSignature m_rs;
		StructuredBuffer m_volumeBuffer;
		CameraSettings m_cameraSettings;
		Settings m_settings;
		float m_time{0.f};
		glm::vec3 m_worldSize{1.f};
	};
}
