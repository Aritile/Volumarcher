#pragma once
#include <string>
#include <glm/vec3.hpp>

#include "../../MiniEngine/Core/Texture.h"


class VoxelWorld
{
public:
	VoxelWorld() = default;

	explicit VoxelWorld(const std::string& _vdbPath, float densityScale = 1.f);

	void LoadVDB(const std::string& _vdb);

	//Scales voxel density (0-1) to this
	void SetDensityScale(const float _densityScale) { m_densityScale = _densityScale; }
	[[nodiscard]] float GetDensityScale() const { return m_densityScale; }

	[[nodiscard]] bool isLoaded() const { return m_loaded; }

	[[nodiscard]] Texture GetDensityField() const { return m_densityField; }
	[[nodiscard]] glm::ivec3 GetSize() const { return m_size; }
private:
	bool m_loaded{false};
	std::vector<float> m_densityVoxels;
	glm::ivec3 m_size;
	Texture m_densityField;
	float m_densityScale{1.f};
};
