#pragma once

#include <vector>

#include "../../MiniEngine/Core/PipelineState.h"
#include "../../MiniEngine/Core/Texture.h"

namespace Imath_3_2
{
	class half;
}

namespace Volumarcher
{
	//Build noise from image
	class DetailNoise
	{
	public:
		///Load 3D noise from data
		///Noise should have low frequency billow noise in red channel, and high frequency in green channel 
		explicit DetailNoise(const Imath_3_2::half* _noiseData, unsigned int _width,
		                     unsigned int _maximumMips = 10);
#ifndef VOLUMARCHER_NO_STB_IMAGE
		///Load 3D noise from images from paths using stb_image
		///Noise should have low frequency billow noise in red channel, and high frequency in green channel 
		explicit DetailNoise(const std::vector<std::string>& _pathPerSlice, unsigned int _maximumMips = 10);
#endif
		~DetailNoise();

	private:
		friend class VolumarcherContext;

		void GenerateMipMaps(unsigned int _amountOfMips);

		[[nodiscard]] Texture GetNoise() { return m_noiseTexture; }

		static ComputePSO m_mipMapPso;
		static RootSignature m_mipMapRS;
		Texture m_noiseTexture;
		static bool m_psoGenerated;
	};
}
