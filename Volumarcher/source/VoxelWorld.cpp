#include "VoxelWorld.h"
#define IMATH_HALF_NO_LOOKUP_TABLE
#include <openvdb/openvdb.h>
#include <openvdb/tools/Dense.h>

#include "../../MiniEngine/Core/Utility.h"

VoxelWorld::VoxelWorld(const std::string& _vdbPath, float _densityScale)
{
	LoadVDB(_vdbPath);
	SetDensityScale(_densityScale);
	return;
}

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
}
