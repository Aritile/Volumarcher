#pragma once
#include "../../MiniEngine/Core/Texture.h"

//Noise to use for a volume
class VolumeNoise
{
public:
	VolumeNoise() = default;


	//Get noise texture, different types in different channels
	[[nodiscard]] virtual Texture GetNoise() = 0;

};
