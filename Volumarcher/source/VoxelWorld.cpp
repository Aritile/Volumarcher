#include "VoxelWorld.h"
#define IMATH_HALF_NO_LOOKUP_TABLE
<<<<<<< HEAD
#pragma warning(push,0)
#include <openvdb/openvdb.h>
#include <openvdb/tools/Dense.h>
#include <openvdb/tools/ChangeBackground.h>
#pragma warning(pop)

#include <glm/detail/type_half.hpp>

#include "CompressedSDF.h"
#include "Volumarcher.h"
#include "../../MiniEngine/Core/CommandContext.h"
#include "../../MiniEngine/Core/PipelineState.h"
#include "../../MiniEngine/Core/RootSignature.h"
#include "../shaders/ShaderCommon.h"

bool Volumarcher::Internal::VoxelWorld::m_ambientPSOExists{false};
ComputePSO Volumarcher::Internal::VoxelWorld::m_ambientDensityPassPSO{};
RootSignature Volumarcher::Internal::VoxelWorld::m_ambientDensityPassRS{};


Volumarcher::Internal::VoxelWorld::VoxelWorld(openvdb::io::File& _vdbFile, const float _maxSdfSize,
                                              const float _densityScale,
                                              const unsigned int _lightCacheVoxelSizeInDataVoxels,
                                              const unsigned int _ambientPreprocessSamples) :
	m_lightCacheVoxelSizeInVoxels(_lightCacheVoxelSizeInDataVoxels),
	m_ambientDensitySamples(_ambientPreprocessSamples)
{
	auto result [[maybe_unused]] = LoadVDB(_vdbFile, _maxSdfSize);
	assert(result == Result::Succeeded);
=======
#include <openvdb/openvdb.h>
#include <openvdb/tools/Dense.h>

#include "../../MiniEngine/Core/Utility.h"

VoxelWorld::VoxelWorld(const std::string& _vdbPath, float _densityScale)
{
	LoadVDB(_vdbPath);
>>>>>>> 57591406dec9de690cf8c3265bfe9e668a263d01
	SetDensityScale(_densityScale);
	return;
}

<<<<<<< HEAD
Volumarcher::Internal::VoxelWorld::~VoxelWorld()
{
	m_densityField.Destroy();
	m_sdfTexture.Destroy();
	m_cachingVoxels.Destroy();
}


Volumarcher::Result Volumarcher::Internal::VoxelWorld::LoadVDB(
	openvdb::io::File& _file, float _maxSdfDistance,
	unsigned int _lightCacheVoxelSizeInDataVoxels, unsigned int _ambientPreprocessSamples)
{
	m_lightCacheVoxelSizeInVoxels = _lightCacheVoxelSizeInDataVoxels;
	m_ambientDensitySamples = _ambientPreprocessSamples;


	m_colorDensGridExists = false;
	m_sdfExists = false;

	if (m_loaded)
	{
		//Make sure to wait until textures are no longer used
		auto& fenceContext = ComputeContext::Begin(L"Voxel Cache Transition");
		fenceContext.TransitionResource(m_densityField, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
		fenceContext.TransitionResource(m_sdfTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true);
		fenceContext.Finish(true);
	}


	openvdb::GridBase::Ptr baseGrid;
	openvdb::GridBase::Ptr sdfGrid;
	openvdb::GridBase::Ptr colorGrid[3];
	openvdb::io::File::NameIterator nameIter = _file.beginName();
	if (++nameIter == _file.endName()) // Only 1 grid
	{
		openvdb::GridBase::Ptr grid = _file.readGrid(_file.beginName().gridName());
		if (grid->isType<openvdb::FloatGrid>())
		{
			baseGrid = grid;
		}
	}
	else
	{
		for (auto iter = _file.beginName(); iter != _file.endName(); ++iter)
		{
			if (iter.gridName() == "dimensional_profile" || iter.gridName() == "density")
			{
				openvdb::GridBase::Ptr grid = _file.readGrid(iter.gridName());
				if (grid->isType<openvdb::FloatGrid>())
				{
					baseGrid = grid;
				}
			}
			else if (iter.gridName() == "sdf" || iter.gridName() == "distance")
			{
				openvdb::GridBase::Ptr grid = _file.readGrid(iter.gridName());
				if (grid->isType<openvdb::FloatGrid>())
				{
					sdfGrid = grid;
				}
			}
			else if (iter.gridName().compare(0, 6, "color.") == 0 && iter.gridName().length() == 7)
			{
				int channel;
				switch (iter.gridName()[6])
				{
				case 'x': channel = 0;
					break;
				case 'y': channel = 1;
					break;
				case 'z': channel = 2;
					break;
				default: continue; // invalid so continue
				}
				openvdb::GridBase::Ptr grid = _file.readGrid(iter.gridName());
				if (grid->isType<openvdb::FloatGrid>())
				{
					colorGrid[channel] = grid;
				}
			}
		}
	}

	assert(baseGrid);
	if (!baseGrid)
	{
		Utility::Printf(
			(std::string(
				"Volumarcher error: VDB file has no dimensional profile grid named: \"density\" or \"dimensional_profile\"\n"))
			.c_str());

		return Result::Failed;
	}

	auto densityGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

	// Convert to a dense grid
	openvdb::CoordBBox densityBBox = densityGrid->evalActiveVoxelBoundingBox();
	openvdb::tools::Dense<float, openvdb::tools::LayoutXYZ> denseDensityGrid(densityBBox);

	memcpy(&m_densitySize, densityBBox.dim().asVec3i().asPointer(), sizeof(glm::ivec3));
	memcpy(&m_densityOrigin, densityBBox.min().asVec3i().asPointer(), sizeof(glm::ivec3));


	// Copy float values from the sparse grid to dense
	openvdb::tools::copyToDense(*densityGrid, denseDensityGrid);

	if (sdfGrid)
	{
		auto distanceGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(sdfGrid);
		openvdb::tools::changeBackground(distanceGrid->tree(), _maxSdfDistance);
		openvdb::CoordBBox distanceBBox = distanceGrid->evalActiveVoxelBoundingBox();
		openvdb::tools::Dense<float, openvdb::tools::LayoutXYZ> denseDistanceGrid(distanceBBox);
		openvdb::tools::copyToDense(*distanceGrid, denseDistanceGrid);
		memcpy(&m_sdfSize, distanceBBox.dim().asVec3i().asPointer(), sizeof(glm::ivec3));
		memcpy(&m_distanceOrigin, distanceBBox.min().asVec3i().asPointer(), sizeof(glm::ivec3));

#if 0
		////No BC
		std::vector<half> distanceData(denseDistanceGrid.valueCount());
		for (int i = 0; i < denseDistanceGrid.valueCount(); ++i)
		{
			distanceData[i] = static_cast<half>(denseDistanceGrid.data()[i]);
		}
		m_sdfTexture.Create3D(m_sdfSize.x * sizeof(half), m_sdfSize.x, m_sdfSize.y, m_sdfSize.z,
			DXGI_FORMAT_R16_FLOAT,
			distanceData.data());
#else
		//Block compression
		std::vector<uint64_t> blockData;
		auto compressedData = CompressedSDFData::GenerateCompressedSDF(_file, _maxSdfDistance);
		size_t rowPitch = compressedData.sizeInBlocks.x * sizeof(uint64_t);
		size_t slicePitch = rowPitch * compressedData.sizeInBlocks.y;
		m_sdfSize = glm::vec3(compressedData.sizeInBlocks.x * 4, compressedData.sizeInBlocks.y * 4,
		                      compressedData.sizeInBlocks.z);


		m_sdfTexture.Create3D(rowPitch, compressedData.sizeInBlocks.x * 4, compressedData.sizeInBlocks.y * 4,
		                      compressedData.sizeInBlocks.z,
		                      DXGI_FORMAT_BC1_UNORM,
		                      compressedData.bc1Data.data(), 1, D3D12_RESOURCE_FLAG_NONE, slicePitch);
#endif

		m_sdfExists = true;
	}


	if ((colorGrid[0] && colorGrid[1] && colorGrid[2]))
	{
		// same size as densityGrid: R,G,B,DensityScale
		std::vector<half> colorDensityData(denseDensityGrid.valueCount() * 4);


		if (colorGrid[0] && colorGrid[1] && colorGrid[2])
		{
			auto rGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(colorGrid[0]);
			auto gGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(colorGrid[1]);
			auto bGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(colorGrid[2]);

			openvdb::tools::Dense<float, openvdb::tools::LayoutXYZ>
				denseRGrid(densityBBox), denseGGrid(densityBBox), denseBGrid(densityBBox);

			openvdb::tools::copyToDense(*rGrid, denseRGrid);
			openvdb::tools::copyToDense(*gGrid, denseGGrid);
			openvdb::tools::copyToDense(*bGrid, denseBGrid);
			for (int i = 0; i < denseDensityGrid.valueCount(); ++i)
			{
				colorDensityData[i * 4 + 0] = static_cast<half>(denseRGrid.data()[i]);
				colorDensityData[i * 4 + 1] = static_cast<half>(denseGGrid.data()[i]);
				colorDensityData[i * 4 + 2] = static_cast<half>(denseBGrid.data()[i]);
			}
		}
		m_colorDensityTexture.Create3D(m_densitySize.x * sizeof(half) * 4, m_densitySize.x, m_densitySize.y,
		                               m_densitySize.z,
		                               DXGI_FORMAT_R16G16B16A16_FLOAT,
		                               colorDensityData.data());
		m_colorDensGridExists = true;
	}


	std::vector<half> densityData(denseDensityGrid.valueCount());
	for (int i = 0; i < denseDensityGrid.valueCount(); ++i)
	{
		densityData[i] = static_cast<half>(denseDensityGrid.data()[i]);
	}
	m_densityField.Create3D(m_densitySize.x * sizeof(half), m_densitySize.x, m_densitySize.y, m_densitySize.z,
	                        DXGI_FORMAT_R16_FLOAT,
	                        densityData.data());


	//Caching voxels
	m_cacheSize = max(m_densitySize / static_cast<int>(m_lightCacheVoxelSizeInVoxels), 1);

	std::vector<half> cacheInitData = std::vector<half>(m_cacheSize.x * m_cacheSize.y * m_cacheSize.z * 2, 0.f);
	m_cachingVoxels.Create3D(m_cacheSize.x * sizeof(half) * 2, m_cacheSize.x, m_cacheSize.y, m_cacheSize.z,
	                         DXGI_FORMAT_R16G16_FLOAT, cacheInitData.data());

	m_cacheUAV = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = m_cachingVoxels->GetDesc().Format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	uavDesc.Texture3D.MipSlice = 0;
	uavDesc.Texture3D.FirstWSlice = 0;
	uavDesc.Texture3D.WSize = (UINT)-1; // full mask
	Graphics::g_Device->CreateUnorderedAccessView(m_cachingVoxels.GetResource(), nullptr, &uavDesc,
	                                              m_cacheUAV);
	CalculateAmbientDensity(m_ambientDensitySamples);
	m_loaded = true;

	Utility::Printf("Volumarcher Info: Succesfully loaded VDB of resolution x%d y%d z%d totalVoxels (density): %d\n",
	                m_densitySize.x,
	                m_densitySize.y, m_densitySize.z, m_densitySize.x * m_densitySize.y * m_densitySize.z);

	return Result::Succeeded;
}


void Volumarcher::Internal::VoxelWorld::RecalculateCachedLighting(const unsigned int _ambientSampleCount)
{
	m_ambientDensitySamples = _ambientSampleCount;
	CalculateAmbientDensity(_ambientSampleCount);
}

void Volumarcher::Internal::VoxelWorld::RecreateLightCache(const unsigned int _lightCacheVoxelSizeInDataVoxels)
{
	assert(_lightCacheVoxelSizeInDataVoxels > 0); // _lightCacheVoxelSize should be > 0
	if (_lightCacheVoxelSizeInDataVoxels <= 0)
	{
		Utility::Printf("WARNING: Volumarcher: Tried to set light cache size to value < 1 which is not possible! \n");
		m_lightCacheVoxelSizeInVoxels = 1u;
	}
	else
	{
		m_lightCacheVoxelSizeInVoxels = _lightCacheVoxelSizeInDataVoxels;
	}


	m_cacheSize = m_densitySize / static_cast<int>(m_lightCacheVoxelSizeInVoxels);

	std::vector<half> cacheInitData = std::vector<half>(m_cacheSize.x * m_cacheSize.y * m_cacheSize.z * 2, 0.f);
	auto& fenceContext = ComputeContext::Begin(L"Cache Transition");
	fenceContext.TransitionResource(m_cachingVoxels, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	fenceContext.Finish(true);

	m_cachingVoxels.Create3D(m_cacheSize.x * sizeof(half) * 2, m_cacheSize.x, m_cacheSize.y, m_cacheSize.z,
	                         DXGI_FORMAT_R16G16_FLOAT, cacheInitData.data());
	m_cacheUAV = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = m_cachingVoxels->GetDesc().Format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	uavDesc.Texture3D.MipSlice = 0;
	uavDesc.Texture3D.FirstWSlice = 0;
	uavDesc.Texture3D.WSize = (UINT)-1; // full mask
	Graphics::g_Device->CreateUnorderedAccessView(m_cachingVoxels.GetResource(), nullptr, &uavDesc,
	                                              m_cacheUAV);
	CalculateAmbientDensity(m_ambientDensitySamples);
}

#include "CompiledShaders/CalcAmbientDensityCS.h"

void Volumarcher::Internal::VoxelWorld::CalculateAmbientDensity(const unsigned int _sampleCount)
{
	if (!m_ambientPSOExists)
	{
		m_ambientDensityPassRS.Reset(3, 1);
		//Output texture
		m_ambientDensityPassRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);

		//Volume texture
		m_ambientDensityPassRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
		//Constants
		m_ambientDensityPassRS[2].InitAsConstants(0, sizeof(AmbientPreprocessSettings) / sizeof(uint32_t));

		//Linear clamp sampler
		D3D12_SAMPLER_DESC profileSamplerDesc{
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, 0, 0, D3D12_COMPARISON_FUNC_NONE,
			{0, 0, 0, 1}, 0, 0
		};
		m_ambientDensityPassRS.InitStaticSampler(0, profileSamplerDesc);


		m_ambientDensityPassRS.Finalize(L"RootSig");
		m_ambientDensityPassPSO.SetRootSignature(m_ambientDensityPassRS);
		m_ambientDensityPassPSO.SetComputeShader(g_pCalcAmbientDensityCS, sizeof(g_pCalcAmbientDensityCS));
		m_ambientDensityPassPSO.Finalize();
	}

	ComputeContext& computeContext = ComputeContext::Begin(L"Ambient density precalculation");
	computeContext.SetPipelineState(m_ambientDensityPassPSO);
	computeContext.SetRootSignature(m_ambientDensityPassRS);
	//Bind output
	computeContext.TransitionResource(m_cachingVoxels, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	computeContext.SetDynamicDescriptor(0, 0, m_cacheUAV);
	computeContext.SetDynamicDescriptor(1, 0, m_densityField.GetSRV());
	//Bind variables
	auto outputSize = ::glm::ivec3(m_densitySize / static_cast<int>(m_lightCacheVoxelSizeInVoxels));
	AmbientPreprocessSettings constants{
		outputSize,
		1.f / static_cast<float>(m_lightCacheVoxelSizeInVoxels),
		static_cast<int>(_sampleCount)
	};


	computeContext.SetConstantArray(2, sizeof(AmbientPreprocessSettings) / sizeof(uint32_t), &constants);

	//End call
	computeContext.Dispatch(
		static_cast<size_t>(ceil(outputSize.x / 8.f)),
		static_cast<size_t>(ceil(outputSize.y / 8.f)),
		static_cast<size_t>(ceil(outputSize.z / 8.f)));

	computeContext.Finish();
=======
void VoxelWorld::LoadVDB(const std::string& _vdb)
{
	openvdb::initialize();

	openvdb::io::File file(_vdb);
	file.open();

	openvdb::GridBase::Ptr baseGrid;
	for (openvdb::io::File::NameIterator nameIter = file.beginName(); nameIter != file.endName(); ++nameIter)
	{
		openvdb::GridBase::Ptr grid = file.readGrid(nameIter.gridName());
		if (grid->isType<openvdb::FloatGrid>())
		{
			baseGrid = grid;
			break;
		}
	}

	if (!baseGrid)
	{
		//TODO: Error handling
		abort();
		return;
	}

	auto floatGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

	// Convert to a dense grid
	openvdb::CoordBBox bbox = floatGrid->evalActiveVoxelBoundingBox();
	openvdb::tools::Dense<float, openvdb::tools::LayoutXYZ> denseGrid(bbox);

	// Copy float values from the sparse grid to dense
	openvdb::tools::copyToDense(*floatGrid, denseGrid);

	m_densityVoxels.resize(denseGrid.valueCount());
	std::memcpy(
		m_densityVoxels.data(),
		denseGrid.data(),
		denseGrid.valueCount() * sizeof(float)
	);
	file.close();

	m_size = *reinterpret_cast<glm::ivec3*>(&bbox.dim().asVec3i());

	m_densityField.Destroy();

	m_densityField.Create3D(m_size.x * sizeof(float), m_size.x, m_size.y, m_size.z, DXGI_FORMAT_R32_FLOAT,
	                        m_densityVoxels.data());

	//std::swap(m_size.y, m_size.z);

	m_loaded = true;
>>>>>>> 57591406dec9de690cf8c3265bfe9e668a263d01
}
