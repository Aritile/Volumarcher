#include "BlueNoise.h"

#include <Imath/half.h>


Volumarcher::BlueNoise::BlueNoise(const Imath_3_2::half* _data, const unsigned int _width,
                                  const unsigned int _height,
                                  const unsigned int _amountOfTextures)
{
	for (unsigned int i = 0; i < _amountOfTextures; ++i)
	{
		const unsigned int offset = _width * _height * i;
		Texture tex;
		tex.Create2D(sizeof(half) * _width, _width, _height, DXGI_FORMAT_R16_FLOAT, &_data[offset]);
		m_noiseTextures.push_back(tex);
	}
}

#ifndef VOLUMARCHER_NO_STB_IMAGE
#include "stb_image.h"

Volumarcher::BlueNoise::BlueNoise(const std::vector<std::string>& _paths)
{
	//Load blue noise
	for (std::string path : _paths)
	{
		int width{0}, height{0}, channels{0};
		float* noise = stbi_loadf(path.c_str(), &width, &height, &channels, 1);
		assert(noise != nullptr); // Failed to load file
		if (noise == nullptr)
		{
			Utility::Printf("Volumarcher error: Failed to load Blue noise image with STB_Image");

			Texture tex;
			float initData = 1.f;
			tex.Create2D(sizeof(half) * 1, 1, 1, DXGI_FORMAT_R16_FLOAT, &initData);
			m_noiseTextures.push_back(tex);
			return;
		}


		std::vector<half> halfData;
		halfData.resize(width * height);
		for (int i = 0; i < width * height; ++i)
		{
			halfData[i] = static_cast<half>(noise[i]);
		}
		Texture tex;
		tex.Create2D(sizeof(half) * width, width, height, DXGI_FORMAT_R16_FLOAT, halfData.data());
		m_noiseTextures.push_back(tex);
	}
}
#endif

Texture Volumarcher::BlueNoise::GetNoise(const float _time)
{
	return m_noiseTextures[static_cast<unsigned int>(round(_time)) % m_noiseTextures.size()];
}
