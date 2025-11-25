#ifndef __HLSL__
#pragma once
#include <glm/vec3.hpp>
using float3 = glm::vec3;
using uint = uint32_t;
#else
#endif

struct VolumetricCamera
{
	float3 camPos;
	float pad0;
	float3 camDir;
};

struct VolumetricSettings
{
	float3 origin;
	int baseSampleCount;
	float3 worldSize;
	int directLightSampleCount;
	int ambientSampleCount;
	uint screenResX;
	uint screenResY;
	float zNear;
	float zFar;
	float vFovAdjust; // tan(Fov(rad) / 2)
};

static const uint VOLUME_AMOUNT = 1;

struct Volume
{
	float3 position;
	float squaredRad;
	float baseDensity;
	float pad0;
	float pad1;
	float pad2;
};
