#include "Volumarcher.h"

#include "../MiniEngine/Core/CommandContext.h"
#include "../MiniEngine/Core/RootSignature.h"
#include "../MiniEngine/Core/PipelineState.h"
#include "../MiniEngine/Core/BufferManager.h"

namespace Volumarcher
{
	static const std::vector<std::string> NOISE_TEXTURE{
		"../assets/noise/NubisVoxelCloudNoise.001.tga",
		"../assets/noise/NubisVoxelCloudNoise.002.tga",
		"../assets/noise/NubisVoxelCloudNoise.003.tga",
		"../assets/noise/NubisVoxelCloudNoise.004.tga",
		"../assets/noise/NubisVoxelCloudNoise.005.tga",
		"../assets/noise/NubisVoxelCloudNoise.006.tga",
		"../assets/noise/NubisVoxelCloudNoise.007.tga",
		"../assets/noise/NubisVoxelCloudNoise.008.tga",
		"../assets/noise/NubisVoxelCloudNoise.009.tga",
		"../assets/noise/NubisVoxelCloudNoise.010.tga",
		"../assets/noise/NubisVoxelCloudNoise.011.tga",
		"../assets/noise/NubisVoxelCloudNoise.012.tga",
		"../assets/noise/NubisVoxelCloudNoise.013.tga",
		"../assets/noise/NubisVoxelCloudNoise.014.tga",
		"../assets/noise/NubisVoxelCloudNoise.015.tga",
		"../assets/noise/NubisVoxelCloudNoise.016.tga",
		"../assets/noise/NubisVoxelCloudNoise.017.tga",
		"../assets/noise/NubisVoxelCloudNoise.018.tga",
		"../assets/noise/NubisVoxelCloudNoise.019.tga",
		"../assets/noise/NubisVoxelCloudNoise.020.tga",
		"../assets/noise/NubisVoxelCloudNoise.021.tga",
		"../assets/noise/NubisVoxelCloudNoise.022.tga",
		"../assets/noise/NubisVoxelCloudNoise.023.tga",
		"../assets/noise/NubisVoxelCloudNoise.024.tga",
		"../assets/noise/NubisVoxelCloudNoise.025.tga",
		"../assets/noise/NubisVoxelCloudNoise.026.tga",
		"../assets/noise/NubisVoxelCloudNoise.027.tga",
		"../assets/noise/NubisVoxelCloudNoise.028.tga",
		"../assets/noise/NubisVoxelCloudNoise.029.tga",
		"../assets/noise/NubisVoxelCloudNoise.030.tga",
		"../assets/noise/NubisVoxelCloudNoise.031.tga",
		"../assets/noise/NubisVoxelCloudNoise.032.tga",
		"../assets/noise/NubisVoxelCloudNoise.033.tga",
		"../assets/noise/NubisVoxelCloudNoise.034.tga",
		"../assets/noise/NubisVoxelCloudNoise.035.tga",
		"../assets/noise/NubisVoxelCloudNoise.036.tga",
		"../assets/noise/NubisVoxelCloudNoise.037.tga",
		"../assets/noise/NubisVoxelCloudNoise.038.tga",
		"../assets/noise/NubisVoxelCloudNoise.039.tga",
		"../assets/noise/NubisVoxelCloudNoise.040.tga",
		"../assets/noise/NubisVoxelCloudNoise.041.tga",
		"../assets/noise/NubisVoxelCloudNoise.042.tga",
		"../assets/noise/NubisVoxelCloudNoise.043.tga",
		"../assets/noise/NubisVoxelCloudNoise.044.tga",
		"../assets/noise/NubisVoxelCloudNoise.045.tga",
		"../assets/noise/NubisVoxelCloudNoise.046.tga",
		"../assets/noise/NubisVoxelCloudNoise.047.tga",
		"../assets/noise/NubisVoxelCloudNoise.048.tga",
		"../assets/noise/NubisVoxelCloudNoise.049.tga",
		"../assets/noise/NubisVoxelCloudNoise.050.tga",
		"../assets/noise/NubisVoxelCloudNoise.051.tga",
		"../assets/noise/NubisVoxelCloudNoise.052.tga",
		"../assets/noise/NubisVoxelCloudNoise.053.tga",
		"../assets/noise/NubisVoxelCloudNoise.054.tga",
		"../assets/noise/NubisVoxelCloudNoise.055.tga",
		"../assets/noise/NubisVoxelCloudNoise.056.tga",
		"../assets/noise/NubisVoxelCloudNoise.057.tga",
		"../assets/noise/NubisVoxelCloudNoise.058.tga",
		"../assets/noise/NubisVoxelCloudNoise.059.tga",
		"../assets/noise/NubisVoxelCloudNoise.060.tga",
		"../assets/noise/NubisVoxelCloudNoise.061.tga",
		"../assets/noise/NubisVoxelCloudNoise.062.tga",
		"../assets/noise/NubisVoxelCloudNoise.063.tga",
		"../assets/noise/NubisVoxelCloudNoise.064.tga",
		"../assets/noise/NubisVoxelCloudNoise.065.tga",
		"../assets/noise/NubisVoxelCloudNoise.066.tga",
		"../assets/noise/NubisVoxelCloudNoise.067.tga",
		"../assets/noise/NubisVoxelCloudNoise.068.tga",
		"../assets/noise/NubisVoxelCloudNoise.069.tga",
		"../assets/noise/NubisVoxelCloudNoise.070.tga",
		"../assets/noise/NubisVoxelCloudNoise.071.tga",
		"../assets/noise/NubisVoxelCloudNoise.072.tga",
		"../assets/noise/NubisVoxelCloudNoise.073.tga",
		"../assets/noise/NubisVoxelCloudNoise.074.tga",
		"../assets/noise/NubisVoxelCloudNoise.075.tga",
		"../assets/noise/NubisVoxelCloudNoise.076.tga",
		"../assets/noise/NubisVoxelCloudNoise.077.tga",
		"../assets/noise/NubisVoxelCloudNoise.078.tga",
		"../assets/noise/NubisVoxelCloudNoise.079.tga",
		"../assets/noise/NubisVoxelCloudNoise.080.tga",
		"../assets/noise/NubisVoxelCloudNoise.081.tga",
		"../assets/noise/NubisVoxelCloudNoise.082.tga",
		"../assets/noise/NubisVoxelCloudNoise.083.tga",
		"../assets/noise/NubisVoxelCloudNoise.084.tga",
		"../assets/noise/NubisVoxelCloudNoise.085.tga",
		"../assets/noise/NubisVoxelCloudNoise.086.tga",
		"../assets/noise/NubisVoxelCloudNoise.087.tga",
		"../assets/noise/NubisVoxelCloudNoise.088.tga",
		"../assets/noise/NubisVoxelCloudNoise.089.tga",
		"../assets/noise/NubisVoxelCloudNoise.090.tga",
		"../assets/noise/NubisVoxelCloudNoise.091.tga",
		"../assets/noise/NubisVoxelCloudNoise.092.tga",
		"../assets/noise/NubisVoxelCloudNoise.093.tga",
		"../assets/noise/NubisVoxelCloudNoise.094.tga",
		"../assets/noise/NubisVoxelCloudNoise.095.tga",
		"../assets/noise/NubisVoxelCloudNoise.096.tga",
		"../assets/noise/NubisVoxelCloudNoise.097.tga",
		"../assets/noise/NubisVoxelCloudNoise.098.tga",
		"../assets/noise/NubisVoxelCloudNoise.099.tga",
		"../assets/noise/NubisVoxelCloudNoise.100.tga",
		"../assets/noise/NubisVoxelCloudNoise.101.tga",
		"../assets/noise/NubisVoxelCloudNoise.102.tga",
		"../assets/noise/NubisVoxelCloudNoise.103.tga",
		"../assets/noise/NubisVoxelCloudNoise.104.tga",
		"../assets/noise/NubisVoxelCloudNoise.105.tga",
		"../assets/noise/NubisVoxelCloudNoise.106.tga",
		"../assets/noise/NubisVoxelCloudNoise.107.tga",
		"../assets/noise/NubisVoxelCloudNoise.108.tga",
		"../assets/noise/NubisVoxelCloudNoise.109.tga",
		"../assets/noise/NubisVoxelCloudNoise.110.tga",
		"../assets/noise/NubisVoxelCloudNoise.111.tga",
		"../assets/noise/NubisVoxelCloudNoise.112.tga",
		"../assets/noise/NubisVoxelCloudNoise.113.tga",
		"../assets/noise/NubisVoxelCloudNoise.114.tga",
		"../assets/noise/NubisVoxelCloudNoise.115.tga",
		"../assets/noise/NubisVoxelCloudNoise.116.tga",
		"../assets/noise/NubisVoxelCloudNoise.117.tga",
		"../assets/noise/NubisVoxelCloudNoise.118.tga",
		"../assets/noise/NubisVoxelCloudNoise.119.tga",
		"../assets/noise/NubisVoxelCloudNoise.120.tga",
		"../assets/noise/NubisVoxelCloudNoise.121.tga",
		"../assets/noise/NubisVoxelCloudNoise.122.tga",
		"../assets/noise/NubisVoxelCloudNoise.123.tga",
		"../assets/noise/NubisVoxelCloudNoise.124.tga",
		"../assets/noise/NubisVoxelCloudNoise.125.tga",
		"../assets/noise/NubisVoxelCloudNoise.126.tga",
		"../assets/noise/NubisVoxelCloudNoise.127.tga",
		"../assets/noise/NubisVoxelCloudNoise.128.tga"
	};


#include "CompiledShaders/VolumetricsCS.h"

	VolumetricContext::VolumetricContext(CameraSettings _cameraSettings,
	                                     Settings _settings) :
		m_grid(),
		m_noise(NOISE_TEXTURE),
		m_cameraSettings(_cameraSettings),
		m_settings(_settings)
	{
		m_rs.Reset(7, 2);
		//Output texture
		m_rs[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
		//Scene depth
		m_rs[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
		//Constants
		m_rs[2].InitAsConstants(0, sizeof(VolumetricDynamics) / sizeof(uint32_t));
		m_rs[3].InitAsConstants(1, sizeof(VolumetricSettings) / sizeof(uint32_t));
		m_rs[6].InitAsConstants(2, sizeof(VolumetricWorld) / sizeof(uint32_t));
		//Noise textures
		m_rs[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, VOLUME_AMOUNT);
		//Volume texture
		m_rs[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1);

		//Linear wrap sampler
		D3D12_SAMPLER_DESC noiseSamplerDesc{
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
			D3D12_TEXTURE_ADDRESS_MODE_MIRROR, D3D12_TEXTURE_ADDRESS_MODE_MIRROR, 0, 0, D3D12_COMPARISON_FUNC_NONE,
			{0, 0, 0, 1}, 0, 0
		};
		m_rs.InitStaticSampler(0, noiseSamplerDesc);
		//Linear clamp sampler
		D3D12_SAMPLER_DESC profileSamplerDesc{
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, 0, 0, D3D12_COMPARISON_FUNC_NONE,
			{0, 0, 0, 1}, 0, 0
		};
		m_rs.InitStaticSampler(1, profileSamplerDesc);


		m_rs.Finalize(L"RootSig");
		m_computePSO.SetRootSignature(m_rs);
		m_computePSO.SetComputeShader(g_pVolumetricsCS, sizeof(g_pVolumetricsCS));
		m_computePSO.Finalize();
	}

	void VolumetricContext::LoadGrid(const std::string& _vdb, glm::vec3 _gridScale, glm::vec3 _origin,
	                                 float _densityScale)
	{
		m_grid.LoadVDB(_vdb);
		m_grid.SetDensityScale(_densityScale);
		m_gridOrigin = _origin;
		m_gridScale = normalize(glm::vec3(m_grid.GetSize())) * 1.f / _gridScale;
		//m_gridScale = _gridScale;
	}


	void VolumetricContext::Update(const float _deltaTime)
	{
		m_time += _deltaTime;
	}


	void VolumetricContext::Render(ColorBuffer _outputBuffer, D3D12_RESOURCE_STATES _outputBufferState,
	                               DepthBuffer _inputDepth, glm::vec3 _camPos,
	                               glm::quat _camRot)
	{
		if (!m_grid.isLoaded())
		{
			Utility::Printf("ERROR: Volumarcher has no grid loaded, use LoadGrid()");
		}

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
		VolumetricDynamics cameraSettings{
			_camPos, m_time, camDir
		};
		VolumetricSettings baseSettings{
			m_gridOrigin, m_settings.baseSampleCount,
			m_gridScale,
			m_settings.lightingSampleCount,
			m_settings.ambientSampleCount,
			screenX, screenY, m_cameraSettings.zNear, m_cameraSettings.zFar,
			tan(glm::radians(m_cameraSettings.vFov) / 2.f)
		};
		VolumetricWorld worldSettings{
			glm::vec3(1, 0, 0.5) * 0.05f,
			m_grid.GetDensityScale()
		};

		computeContext.SetConstantArray(2, sizeof(VolumetricDynamics) / sizeof(uint32_t), &cameraSettings);


		computeContext.SetConstantArray(3, sizeof(VolumetricSettings) / sizeof(uint32_t), &baseSettings);
		computeContext.SetConstantArray(6, sizeof(VolumetricWorld) / sizeof(uint32_t), &worldSettings);

		//  Noise textures
		computeContext.SetDynamicDescriptor(4, 0, m_noise.GetNoise().GetSRV());
		//Volumes texture
		computeContext.SetDynamicDescriptor(5, 0, m_grid.GetDensityField().GetSRV());

		//End call
		computeContext.Dispatch(ceil(screenX / 32.f), ceil(screenY / 32.f), 1);

		computeContext.TransitionResource(_outputBuffer, _outputBufferState, false);


		computeContext.Finish();
	}
}
