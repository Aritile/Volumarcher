#pragma once
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

#include "Core/Texture.h"

#define IMATH_HALF_NO_LOOKUP_TABLE
#pragma warning(push,0)
#include <openvdb/io/File.h>
#pragma warning(pop)

namespace Volumarcher::Internal
{
	struct CompressedSDFData
	{
		std::vector<uint64_t> bc1Data;
		glm::ivec3 sizeInBlocks;
		glm::ivec3 minBounds;

		/// <summary>
		/// Generates a Compressed BC1 SDF
		/// </summary>
		/// <param name="_vdbFile">VDB File that contains at least 1 SDF floatgrid called "sdf" or "distance"</param>
		/// <param name="_sdfMaxValue">Maximum value inside the SDF, for ones created with the houdini asset this is halfband voxel size, which is 10 by default</param>
		/// <returns>Compressed SDF Data</returns>
		static CompressedSDFData GenerateCompressedSDF(openvdb::io::File& _vdbFile, float _sdfMaxValue);
	};
}
