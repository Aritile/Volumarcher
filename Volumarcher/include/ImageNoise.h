#pragma once

#include <vector>

#include "VolumeNoise.h"
#include "../../MiniEngine/Core/PipelineState.h"
#include "../../MiniEngine/Core/Texture.h"
#include "glm/glm.hpp"

//Build noise from image
class ImageNoise :
	public VolumeNoise
{
public:
	explicit ImageNoise(const std::vector<std::string>& _pathPerSlice, unsigned int _maximumMips = 10);

	[[nodiscard]] Texture GetNoise() override { return m_noiseTexture; }

private:
	static ComputePSO m_mipMapPso;
	static RootSignature m_mipMapRS;
	static bool m_psoGenerated;

	void GenerateMipMaps(unsigned int _amountOfMips);
	Texture m_noiseTexture;
};
