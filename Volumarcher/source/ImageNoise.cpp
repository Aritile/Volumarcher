#include "ImageNoise.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


ImageNoise::ImageNoise(const std::vector<std::string>& _pathPerSlice)
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
	m_noiseTexture.Create3D(w * sizeof(float) * 4, w, h, _pathPerSlice.size(), DXGI_FORMAT_R32G32B32A32_FLOAT,
	                        data.data());
}
