#pragma once
#include <string>
#include <glm/vec3.hpp>

#include "../../MiniEngine/Core/Texture.h"


class VoxelWorld
{
public:
	VoxelWorld() = default;
	explicit VoxelWorld(const std::string& _vdbPath);

	void LoadVDB(const std::string& _vdb);

	[[nodiscard]] bool isLoaded() const { return m_loaded; }

	[[nodiscard]] Texture GetDensityField() const { return m_densityField; }
	[[nodiscard]] glm::ivec3 GetSize() const { return m_size; }
private:
	bool m_loaded{false};
	std::vector<float> m_densityVoxels;
	glm::ivec3 m_size;
	Texture m_densityField;
};
