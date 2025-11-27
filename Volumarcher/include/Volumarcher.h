#pragma once
#include "CloudNoise.h"
#include "CloudNoise.h"
#include "CloudNoise.h"
#include "CloudNoise.h"
#include "CloudNoise.h"
#include "CloudNoise.h"
#include "CloudNoise.h"
#include "CloudNoise.h"
#include "CloudNoise.h"
#include "CloudNoise.h"
#include "../MiniEngine/Core/CommandContext.h"
#include "../MiniEngine/Core/RootSignature.h"
#include "../MiniEngine/Core/PipelineState.h"
#include "../MiniEngine/Core/BufferManager.h"
#include "../shaders/ShaderCommon.h"


#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "CloudNoise.h"
#include "ImageNoise.h"
#include "VoxelWorld.h"

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
		int lightingSampleCount = 16;
		int ambientSampleCount = 5;
	};

	class VolumetricContext
	{
	public:
		VolumetricContext() = delete;

		explicit VolumetricContext(CameraSettings _cameraSettings,
		                           Settings _settings = {});


		void LoadGrid(const std::string& _vdb, glm::vec3 _gridScale = glm::vec3(1.f),
		              glm::vec3 _origin = glm::vec3(0.f), float _densityScale = 1.f);

		[[nodiscard]] Settings GetSettings() const { return m_settings; }
		void SetSettings(const Settings _volumetricSettings) { m_settings = _volumetricSettings; }

		void Update(float _deltaTime);

		void Render(ColorBuffer _outputBuffer, D3D12_RESOURCE_STATES _outputBufferState, DepthBuffer _inputDepth,
		            glm::vec3 _camPos = glm::vec3{0.f},
		            glm::quat _camRot = glm::identity<glm::quat>());

	private:
		//TODO: Make this user specified 
		VoxelWorld m_grid;
		ImageNoise m_noise;

		ComputePSO m_computePSO;
		RootSignature m_rs;
		CameraSettings m_cameraSettings;
		Settings m_settings;
		glm::vec3 m_gridOrigin{0.f};
		glm::vec3 m_gridScale{0.f};
		float m_time{0.f};
	};
}
