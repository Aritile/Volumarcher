#include "Volumarcher.h"

#include "../MiniEngine/Core/CommandContext.h"
#include "../MiniEngine/Core/RootSignature.h"
#include "../MiniEngine/Core/PipelineState.h"
#include "../MiniEngine/Core/BufferManager.h"

namespace Volumarcher
{
#include "CompiledShaders/VolumetricsCS.h"

	VolumetricContext::VolumetricContext(Volume _volumes[VOLUME_AMOUNT], CameraSettings _cameraSettings,
	                                     Settings _settings) :
		m_noise({256, 256, 256}),
		m_cameraSettings(_cameraSettings),
		m_settings(_settings)
	{
		m_rs.Reset(6, 1);
		//Output texture
		m_rs[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
		//Scene depth
		m_rs[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
		//Constants
		m_rs[2].InitAsConstants(0, sizeof(VolumetricCameraSettings) / sizeof(uint32_t));
		m_rs[3].InitAsConstants(1, sizeof(VolumetricCameraSettings) / sizeof(uint32_t));
		//Volume buffer
		m_rs[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
		//Noise textures
		m_rs[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, VOLUME_AMOUNT);

		//Linear wrap sampler
		D3D12_SAMPLER_DESC noiseSamplerDesc{
			D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
			D3D12_TEXTURE_ADDRESS_MODE_MIRROR, D3D12_TEXTURE_ADDRESS_MODE_MIRROR, 0, 0, D3D12_COMPARISON_FUNC_NONE,
			{0, 0, 0, 1}, 0, 0
		};
		m_rs.InitStaticSampler(0, noiseSamplerDesc);

		m_rs.Finalize(L"RootSig");
		m_computePSO.SetRootSignature(m_rs);
		m_computePSO.SetComputeShader(g_pVolumetricsCS, sizeof(g_pVolumetricsCS));
		m_computePSO.Finalize();

		m_volumeBuffer.Create(L"Volume buffer", VOLUME_AMOUNT, sizeof(Volume), &_volumes[0]);
	}

	void VolumetricContext::SetVolumes(Volume _volumes[VOLUME_AMOUNT])
	{
		m_volumeBuffer.Destroy();
		m_volumeBuffer.Create(L"Volume buffer", VOLUME_AMOUNT, sizeof(Volume), &_volumes[0]);
	}

	void VolumetricContext::Render(ColorBuffer _outputBuffer, D3D12_RESOURCE_STATES _outputBufferState,
	                               DepthBuffer _inputDepth, glm::vec3 _camPos,
	                               glm::quat _camRot)
	{
		ComputeContext& computeContext = ComputeContext::Begin(L"Volumetric Pass");
		computeContext.SetPipelineState(m_computePSO);
		computeContext.SetRootSignature(m_rs);
		//Bind output
		computeContext.TransitionResource(_outputBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
		computeContext.SetDynamicDescriptor(0, 0, _outputBuffer.GetUAV());
		computeContext.SetDynamicDescriptor(1, 0, _inputDepth.GetDepthSRV());
		//Bind variables
		auto screenX = _outputBuffer.GetWidth();
		auto screenY = _outputBuffer.GetHeight();

		glm::vec3 camDir = _camRot * glm::vec3(0, 0, 1);
		VolumetricCameraSettings cameraSettings{
			_camPos, screenX, camDir, screenY, m_cameraSettings.zNear, m_cameraSettings.zFar, m_cameraSettings.vFov
		};
		VolumetricSettings baseSettings{
			m_settings.baseSampleCount, m_settings.lightingSampleCount, m_settings.ambientSampleCount
		};


		computeContext.SetConstantArray(2, sizeof(VolumetricCameraSettings) / sizeof(uint32_t), &cameraSettings);
		computeContext.SetConstantArray(3, sizeof(VolumetricSettings) / sizeof(uint32_t), &baseSettings);

		//Bind volumes
		computeContext.SetDynamicDescriptor(4, 0, m_volumeBuffer.GetSRV());
		//  Noise textures
		computeContext.SetDynamicDescriptor(5, 0, m_noise.GetBillowNoise().GetSRV());

		//End call
		computeContext.Dispatch(ceil(screenX / 32.f), ceil(screenY / 32.f), 1);

		computeContext.TransitionResource(_outputBuffer, _outputBufferState, false);


		computeContext.Finish();
	}
}
