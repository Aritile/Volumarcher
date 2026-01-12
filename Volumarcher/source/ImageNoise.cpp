#include "ImageNoise.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "../../MiniEngine/Core/CommandContext.h"
#include "../../MiniEngine/Core/GraphicsCommon.h"
#include "../shaders/ShaderCommon.h"

bool ImageNoise::m_psoGenerated{false};
ComputePSO ImageNoise::m_mipMapPso{};
RootSignature ImageNoise::m_mipMapRS{};

ImageNoise::ImageNoise(const std::vector<std::string>& _pathPerSlice, const unsigned int _maximumMips)
{
	std::vector<float> data;
	int w{0}, h{0};
	for (std::string path : _pathPerSlice)
	{
		int width{0}, height{0}, channels{0};
		float* noise = stbi_loadf(path.c_str(), &width, &height, &channels, 4);
		data.insert(data.end(), noise, noise + width * height * channels);

		if (w != 0 && h != 0 && (width != w || height != h))
		{
			Utility::Printf("CRITICAL: Loading imagenoise with slices with differing resolutions");
			assert(false);
			float fallbackData{0.f};
			m_noiseTexture.Create3D(sizeof(float), 1, 1, 1, DXGI_FORMAT_R32_FLOAT, &fallbackData);
			return;
		}
		w = width;
		h = height;
	}

	const int mips = std::min(static_cast<int>(log2(w)), static_cast<int>(_maximumMips));

	m_noiseTexture.Create3D(w * sizeof(float) * 4, w, h, _pathPerSlice.size(), DXGI_FORMAT_R32G32B32A32_FLOAT,
	                        data.data(), mips);
	GenerateMipMaps(mips);
}

#include "CompiledShaders/Generate3DMipsCS.h"

void ImageNoise::GenerateMipMaps(unsigned int _amountOfMips)
{
	ComputeContext& context = ComputeContext::Begin(L"Mipmap generation");
	if (!m_psoGenerated)
	{
		//Create root sig
		m_mipMapRS.Reset(3, 1);
		//Output texture
		m_mipMapRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
		//Input texture
		m_mipMapRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
		//Constants
		m_mipMapRS[2].InitAsConstants(0, sizeof(MipConstants) / sizeof(uint32_t));

		//Linear clamp sampler
		D3D12_SAMPLER_DESC mipSampler{
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, 0, 0, D3D12_COMPARISON_FUNC_NONE,
			{0, 0, 0, 1}, 0, 0
		};
		m_mipMapRS.InitStaticSampler(0, mipSampler);
		m_mipMapRS.Finalize(L"RootSig");

		m_mipMapPso.SetRootSignature(m_mipMapRS);
		m_mipMapPso.SetComputeShader(g_pGenerate3DMipsCS, sizeof(g_pGenerate3DMipsCS));
		m_mipMapPso.Finalize();
	}
	context.SetPipelineState(m_mipMapPso);
	context.SetRootSignature(m_mipMapRS);
	//Bind output


	//Create output

	D3D12_CPU_DESCRIPTOR_HANDLE lastHandle = m_noiseTexture.GetSRV();
	for (int mip = 1; mip < _amountOfMips; ++mip)
	{
		context.TransitionResource(m_noiseTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
		//TODO: FIX MEMORY LEAK
		D3D12_CPU_DESCRIPTOR_HANDLE handle = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = m_noiseTexture->GetDesc().Format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.MipSlice = mip;
		uavDesc.Texture3D.FirstWSlice = 0;
		uavDesc.Texture3D.WSize = (UINT)-1; // full mask
		Graphics::g_Device->CreateUnorderedAccessView(m_noiseTexture.GetResource(), nullptr, &uavDesc,
		                                              handle);

		context.SetDynamicDescriptor(0, 0, handle);
		context.SetDynamicDescriptor(1, 0, lastHandle);
		//Bind variables

		unsigned int srcSize = m_noiseTexture.GetWidth(); // texture should be perfect cube
		unsigned int outSize = srcSize >> mip;
		MipConstants constants{
			static_cast<int>(outSize)
		};


		context.SetConstantArray(2, sizeof(MipConstants) / sizeof(uint32_t), &constants);

		context.Dispatch(ceil(outSize / 8.f), ceil(outSize / 8.f), ceil(outSize / 8.f));

		lastHandle = handle;
	}

	context.Finish();
}
