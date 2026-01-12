#include "CompressedSDF.h"

#define IMATH_HALF_NO_LOOKUP_TABLE
#pragma warning(push,0)
#include <openvdb/openvdb.h>
#include <openvdb/tools/Dense.h>
#include <openvdb/tools/ChangeBackground.h>
#pragma warning(pop)
#include "Core/CommandContext.h"

#include "../shaders/ShaderCommon.h"
#include "Core/ReadbackBuffer.h"

#include "CompiledShaders/BlockCompressSdfCS.h"

Volumarcher::Internal::CompressedSDFData Volumarcher::Internal::CompressedSDFData::GenerateCompressedSDF(
	openvdb::io::File& _vdbFile, float _sdfMaxValue)
{
	openvdb::GridBase::Ptr sdfGrid;
	for (auto iter = _vdbFile.beginName(); iter != _vdbFile.endName(); ++iter)
	{
		if (iter.gridName() == "sdf" || iter.gridName() == "distance")
		{
			openvdb::GridBase::Ptr grid = _vdbFile.readGrid(iter.gridName());
			if (grid->isType<openvdb::FloatGrid>())
			{
				sdfGrid = grid;
			}
		}
	}
	assert(sdfGrid);
	if (!sdfGrid)
	{
		Utility::Printf(
			(std::string(
				"Volumarcher error: VDB file has no SDF grid named: \"sdf\" or \"distance\"\n"))
			.c_str());

		return {};
	}

	auto distanceGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(sdfGrid);
	openvdb::tools::changeBackground(distanceGrid->tree(), _sdfMaxValue);
	openvdb::CoordBBox distanceBBox = distanceGrid->evalActiveVoxelBoundingBox();

	openvdb::tools::Dense<float, openvdb::tools::LayoutXYZ> denseDistanceGrid(distanceBBox);
	openvdb::tools::copyToDense(*distanceGrid, denseDistanceGrid);

	//Create raw source texture

	std::vector<half> distanceData(denseDistanceGrid.valueCount());
	for (int i = 0; i < denseDistanceGrid.valueCount(); ++i)
	{
		distanceData[i] = static_cast<half>(denseDistanceGrid.data()[i]);
	}
	Texture sourceTexture;
	sourceTexture.Create3D(distanceBBox.dim().x() * sizeof(half), distanceBBox.dim().x(), distanceBBox.dim().y(),
	                       distanceBBox.dim().z(),
	                       DXGI_FORMAT_R16_FLOAT,
	                       distanceData.data());


	const int dimX = distanceBBox.dim().x();
	const int dimY = distanceBBox.dim().y();
	const int dimZ = distanceBBox.dim().z();

	const int blocksX = (dimX + 3) / 4;
	const int blocksY = (dimY + 3) / 4;

	static bool psoCreated{false};
	static ComputePSO pso;
	static RootSignature rs;
	if (!psoCreated)
	{
		rs.Reset(3, 1);
		//Buffers
		rs[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1); // Output Bc1
		rs[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1); // Input sdf

		//Settings
		rs[2].InitAsConstants(0, sizeof(CompressConstants) / sizeof(uint32_t));

		D3D12_SAMPLER_DESC sampler{
			D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 0.f, 0, D3D12_COMPARISON_FUNC_NONE, {0.f, 0.f, 0.f, 0.f}, 0, 0
		};

		rs.InitStaticSampler(0, sampler);

		rs.Finalize(L"Volumarcher: Blit RS");

		pso.SetRootSignature(rs);
		pso.SetComputeShader(g_pBlockCompressSdfCS, sizeof(g_pBlockCompressSdfCS));
		pso.Finalize();

		psoCreated = true;
	}


	auto& context = ComputeContext::Begin(L"BC1 Sdf compression");


	context.SetPipelineState(pso);
	context.SetRootSignature(rs);

	//Create buffer for output
	StructuredBuffer outputBuffer;
	outputBuffer.Create(L"Compressed SDF output data", blocksX * blocksY * dimZ, sizeof(unsigned int) * 2);

	//Create buffer for readback
	ReadbackBuffer readbackBuffer;
	readbackBuffer.Create(L"Compressed SDF readback", blocksX * blocksY * dimZ, sizeof(unsigned int) * 2);

	context.TransitionResource(outputBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	context.TransitionResource(sourceTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true);
	context.SetDynamicDescriptor(0, 0, outputBuffer.GetUAV());
	context.SetDynamicDescriptor(1, 0, sourceTexture.GetSRV());

	CompressConstants constants{
		glm::ivec3(blocksX, blocksY, dimZ),
		1.f / _sdfMaxValue
	};

	context.SetConstantArray(2, sizeof(constants) / sizeof(uint32_t), &constants);
	static constexpr int GROUP_SIZE = COMPRESSION_GROUP_SIZE;
	context.Dispatch3D(blocksX, blocksY, dimZ, GROUP_SIZE, GROUP_SIZE, 1);

	context.TransitionResource(outputBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, false);

	context.CopyBuffer(readbackBuffer, outputBuffer);

	context.Finish(true);

	auto* bcData = static_cast<const uint64_t*>(readbackBuffer.Map());

	CompressedSDFData result;
	result.bc1Data.resize(blocksX * blocksY * dimZ);
	memcpy(result.bc1Data.data(), bcData, blocksX * blocksY * dimZ * sizeof(uint64_t));
	result.sizeInBlocks = {blocksX, blocksY, dimZ};
	memcpy(&result.minBounds, distanceBBox.min().asVec3i().asPointer(), sizeof(glm::ivec3));
	return std::move(result);
}
