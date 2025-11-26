#pragma once

#include <vector>

#include "VolumeNoise.h"
#include "../../MiniEngine/Core/Texture.h"
#include "glm/glm.hpp"

//Build noise from image
class ImageNoise :
	public VolumeNoise
{
public:
	explicit ImageNoise(const std::vector<std::string>& _pathPerSlice);

	[[nodiscard]] Texture GetNoise() override { return m_noiseTexture; }

private:
	Texture m_noiseTexture;
};
