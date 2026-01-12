#pragma once
#include <string>
#include <vector>

#include "../../MiniEngine/Core/Texture.h"

namespace Imath_3_2
{
	class half;
}

namespace Volumarcher
{
	class BlueNoise
	{
	public:
		/// Load blue noise from data
		explicit BlueNoise(const Imath_3_2::half* _data, unsigned int _width, unsigned int _height,
		                   unsigned int _amountOfTextures);

#ifndef VOLUMARCHER_NO_STB_IMAGE

		/// Load blue noise from path using STB image
		explicit BlueNoise(const std::vector<std::string>& _paths);
#endif
	private:
		friend class VolumarcherContext;

		Texture GetNoise(float _time);

		std::vector<Texture> m_noiseTextures;
	};
}
