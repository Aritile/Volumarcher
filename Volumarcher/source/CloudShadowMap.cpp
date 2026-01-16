#include "CloudShadowMap.h"

#include "Volumarcher.h"
#include "../shaders/ShaderCommon.h"
#include "Core/GraphicsCore.h"


ComputePSO Volumarcher::CloudShadowMap::m_shadowMapPSO{L"Volumarcher: Shadow map PSO"};
RootSignature Volumarcher::CloudShadowMap::m_shadowMapRs{};
bool Volumarcher::CloudShadowMap::m_createdPSO{false};

#include "CompiledShaders/ShadowMarcher.h"

Volumarcher::CloudShadowMap::CloudShadowMap(const unsigned int _resolution)
{
	auto initData = std::vector<float>(_resolution * _resolution, 1.f);
	m_map.Create2D(_resolution * sizeof(float), _resolution, _resolution, DXGI_FORMAT_R32_FLOAT, initData.data(),
	               D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	//Create write UAV
	m_uav = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = m_map->GetDesc().Format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture3D.MipSlice = 0;
	uavDesc.Texture3D.FirstWSlice = 0;
	uavDesc.Texture3D.WSize = (UINT)-1; // full mask
	Graphics::g_Device->CreateUnorderedAccessView(m_map.GetResource(), nullptr, &uavDesc, m_uav);

	//Create PSO
	if (!m_createdPSO)
	{
		//PSO for main render
		{
			m_shadowMapRs.Reset(4, 1);
			//Buffers
			m_shadowMapRs[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
			m_shadowMapRs[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);

			//Settings
			m_shadowMapRs[2].InitAsConstants(0, sizeof(ShadowMapConstants) / sizeof(uint32_t));

			m_shadowMapRs[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

			//Linear clamp sampler
			D3D12_SAMPLER_DESC profileSamplerDesc{
				D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 0, D3D12_COMPARISON_FUNC_NONE,
				{0, 0, 0, 1}, 0, 0
			};
			m_shadowMapRs.InitStaticSampler(0, profileSamplerDesc);

			m_shadowMapRs.Finalize(L"Volumarcher: Shadow map RS");

			m_shadowMapPSO.SetRootSignature(m_shadowMapRs);
			m_shadowMapPSO.SetComputeShader(g_pShadowMarcher, sizeof(g_pShadowMarcher));
			m_shadowMapPSO.Finalize();
		}
	}
}

void Volumarcher::CloudShadowMap::RenderMapShadowing(VolumarcherContext& _context, ShadowMapSettings _settings,
                                                     const D3D12_CPU_DESCRIPTOR_HANDLE _baseShadowMap)
{
	ComputeContext& computeContext = ComputeContext::Begin(L"Volumarcher: Render shadow map");
	computeContext.SetPipelineState(m_shadowMapPSO);
	computeContext.SetRootSignature(m_shadowMapRs);

	//Bind output
	computeContext.TransitionResource(m_map, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	computeContext.SetDynamicDescriptor(0, 0, m_uav);
	computeContext.SetDynamicDescriptor(1, 0, _baseShadowMap);
	//Bind variables
	unsigned int res = m_map.GetWidth();

	ShadowMapConstants constants{
		{
			glm::vec4(_settings.view[0], 0.f),
			glm::vec4(_settings.view[1], 0.f),
			glm::vec4(_settings.view[2], 0.f),
		},
		_settings.cameraPos,
		res,
		_context.m_densityGridOrigin,
		_settings.nearPlane,
		_context.m_densityGridScale,
		_settings.farPlane,
		_context.m_distanceGridOrigin,
		1.0f - _settings.historyPreservation,
		_context.m_distanceGridScale,
		_context.m_cloudLookSettings.cloudOffset.x,
		_context.m_cloudLookSettings.cloudOffset.y,
		_context.m_grid.GetDensityScale(),
		_settings.orthoSizeX,
		_settings.orthoSizeY,
		_settings.sampleCount,

		static_cast<float>(_context.m_time)
	};

	computeContext.SetConstantArray(2,
	                                sizeof(ShadowMapConstants) / sizeof(uint32_t), &constants);
	//Cache data
	computeContext.SetDynamicDescriptor(3, 0, _context.m_grid.GetDensityField().GetSRV());

	//End call
	static constexpr int GROUP_SIZE = SHADOW_PASS_GROUP_SIZE;
	computeContext.Dispatch2D(res, res, GROUP_SIZE, GROUP_SIZE);


	computeContext.Finish();
}
