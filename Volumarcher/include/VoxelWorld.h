#pragma once
#include <string>
#include <glm/vec3.hpp>

#define IMATH_HALF_NO_LOOKUP_TABLE
#pragma warning(push,0)
#include <openvdb/io/File.h>
#pragma warning(pop)

#include "../../MiniEngine/Core/GpuBuffer.h"
#include "../../MiniEngine/Core/PipelineState.h"
#include "../../MiniEngine/Core/Texture.h"

namespace Volumarcher
{
	enum class Result : uint8_t;
}

namespace Volumarcher::Internal
{
	class VoxelWorld
	{
	public:
		VoxelWorld() = default;

		explicit VoxelWorld(openvdb::io::File& _vdbPath, float _maxSdfDistance, float uniformDensityScale = 1.f,
		                    unsigned int _lightCacheVoxelSizeInDataVoxels = 2u,
		                    unsigned int _ambientPreprocessSamples = 8u);

		~VoxelWorld();

		//Load clouds from VDB file
		// File should either contain 1 density profile floatgrid
		// Or multiple grids:
		//	REQUIRED: "dimensional_profile" or "density"
		//	OPTIONAL: "distance" or "sdf"     (containing distance to closest > 0 profile)
		//	OPTIONAL: "density_scale" or "scale"     (scalar of the density)
		//	OPTIONAL: "color.x" + "color.y" + "color.z"     (absorption/scatter color multiplier)
		[[nodiscard]] Result LoadVDB(openvdb::io::File& _file, float _maxSdfDistance,
		                             unsigned int _lightCacheVoxelSizeInDataVoxels = 2u,
		                             unsigned int _ambientPreprocessSamples = 8u);


		static void GetCompressedSDF(openvdb::io::File& _file, float _maxDistance, std::vector<uint64_t>& _outBlockData,
		                             glm::ivec3& _outSize);

		//Scales voxel density (0-1) to this
		void SetDensityScale(const float _densityScale) { m_densityScale = _densityScale; }
		[[nodiscard]] float GetDensityScale() const { return m_densityScale; }

		[[nodiscard]] bool IsLoaded() const { return m_loaded; }

		[[nodiscard]] Texture GetDensityField() const { return m_densityField; }
		[[nodiscard]] Texture GetDistanceField() const { return m_sdfTexture; }
		[[nodiscard]] Texture GetLightCache() const { return m_cachingVoxels; }
		[[nodiscard]] Texture GetColorDensityScaleField() const { return m_colorDensityTexture; }
		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetLightCacheUAV() const { return m_cacheUAV; }
		[[nodiscard]] bool HasDistanceField() const { return m_sdfExists; }
		[[nodiscard]] bool HasColorDensityScaleField() const { return m_colorDensGridExists; }
		[[nodiscard]] glm::ivec3 GetDensityGridSize() const { return m_densitySize; }
		[[nodiscard]] glm::ivec3 GetDensityGridOrigin() const { return m_densityOrigin; }
		[[nodiscard]] glm::ivec3 GetDistanceGridSize() const { return m_sdfSize; }
		[[nodiscard]] glm::ivec3 GetDistanceGridOrigin() const { return m_distanceOrigin; }
		[[nodiscard]] unsigned int GetCacheVoxelSize() const { return m_lightCacheVoxelSizeInVoxels; }

		void RecalculateCachedLighting(unsigned int _ambientSampleCount);

		void RecreateLightCache(unsigned int _lightCacheVoxelSizeInDataVoxels);

	private:
		void CalculateAmbientDensity(unsigned int _sampleCount);

		static ComputePSO m_ambientDensityPassPSO;
		static RootSignature m_ambientDensityPassRS;
		static bool m_ambientPSOExists;
		glm::ivec3 m_densitySize;
		glm::ivec3 m_sdfSize;
		glm::ivec3 m_densityOrigin;
		glm::ivec3 m_distanceOrigin;
		glm::ivec3 m_cacheSize;
		Texture m_cachingVoxels;
		D3D12_CPU_DESCRIPTOR_HANDLE m_cacheUAV{0};
		Texture m_densityField;
		Texture m_sdfTexture;
		Texture m_colorDensityTexture;
		unsigned int m_lightCacheVoxelSizeInVoxels = 2u; // How many gridVoxels to cache in 1 lightCache Voxel
		unsigned int m_ambientDensitySamples = 8u; // How many samples to use for ambient density preprocessing
		float m_densityScale{1.f};
		bool m_sdfExists{false};
		bool m_colorDensGridExists{false};
		bool m_loaded{false};
	};
}
