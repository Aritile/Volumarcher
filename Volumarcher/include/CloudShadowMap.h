#pragma once
#include "glm/gtc/quaternion.hpp"
#include <glm/vec3.hpp>

#include "../../MiniEngine/Core/GpuBuffer.h"
#include "../../MiniEngine/Core/PipelineState.h"
#include "../../MiniEngine/Core/Texture.h"
#include "../../MiniEngine/Core/DepthBuffer.h"


namespace Volumarcher
{
	class VolumarcherContext;

	struct ShadowMapSettings
	{
		glm::vec3 cameraPos;
		glm::mat3 view;

		float nearPlane;
		float farPlane;
		float orthoSizeX;
		float orthoSizeY;

		//Amount of samples to take
		unsigned int sampleCount = 64;

		//Integrates map over multiple passes
		float historyPreservation = 0.f;
	};

	class CloudShadowMap
	{
	public:
		explicit CloudShadowMap(unsigned int _resolution);

		//THIS IS A WIP FEATURE
		void RenderMapShadowing(VolumarcherContext& _context, ShadowMapSettings _settings,
		                        const D3D12_CPU_DESCRIPTOR_HANDLE _baseShadowMap);

		[[nodiscard]] Texture& GetShadowStrengthMap() { return m_map; }

	private:
		Texture m_map;
		D3D12_CPU_DESCRIPTOR_HANDLE m_uav;

		static ComputePSO m_shadowMapPSO;
		static RootSignature m_shadowMapRs;
		static bool m_createdPSO;
	};
}
