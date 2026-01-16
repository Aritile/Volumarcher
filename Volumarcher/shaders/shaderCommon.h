#ifndef __HLSL__
#pragma once
#include <glm/vec3.hpp>
#include "glm/glm.hpp"
using float3 = glm::vec3;
using float4x4 = glm::mat4;

struct mat3
{
	glm::vec4 m[3];
};

using float3x3 = mat3;
using int3 = glm::ivec3;
using uint = uint32_t;
#else
#endif


static const uint MAX_LIGHTSOURCES = 4;
static const uint VOLUMETRIC_PASS_GROUP_SIZE = 16;
static const uint SHADOW_PASS_GROUP_SIZE = 16;
static const uint COPY_GROUP_SIZE = 8;
static const uint COMPRESSION_GROUP_SIZE = 16;

struct BlitValues
{
	uint outputX;
	uint outputY;
};

struct VolumetricDynamics
{
	float3 camPos;
	float time;
	float3 camDir;
	float pad0;
	float3 oldCamPos;
	int amountOfLights;
	float3 oldCamDir;
	float deltaTime;
};

struct VolumetricSettings
{
	float3 densityGridOrigin;
	int baseSampleCount;

	float3 densityTextureScale;
	int directLightTotalSampleCount;

	float3 sdfGridOrigin;
	float targetRenderDistance;

	float3 sdfTextureScale;
	uint screenResX;

	uint screenResY;
	float zNear;
	float zFar;
	float vFovAdjust; // tan(Fov(rad) / 2)

	float sdfScale;
	float noiseScale;
	float ambientScale;
	float sdfMaxValue;

	int directLightRealSampleCount;
	float directLightingFarPlane;
	float vFov;
	float adaptiveStepOverDistance;

	float resolutionScale;
	float aspect;
	float reprojectAmount;
	int hasColorDensityGrid;
};


struct AmbientPreprocessSettings
{
	int3 outputSize;
	float gridScale;

	int sampleCount;
};


struct VolumetricWorld
{
	float3 wind; // Direction/speed the noise scrolls
	float globalDensityScale; // scale the density from world (0-1) with this
	float3 sunDir;
	float cloudEccentricity;
	float3 sunLight;
	float offsetX;
	float3 cloudAbsorptionColor;
	float offsetZ;
	float3 ambientLight;
	float pad2;
};

struct MipConstants
{
	int outSize;
};

struct CompressConstants
{
	int3 outSize;
	float sdfMaxSizeReciprical;
};


struct DirectLightingSettings
{
	int3 outputSize;
	float gridScale;

	float3 sunDir;
	int sampleCount;

	float3 wind;
	float time;

	int3 dispatchOffset;
	float noiseScale;

	float farPlane;
	float sdfScale;
	float pad1;
	float pad2;
};

struct AtmosphereRenderConstants
{
	float3 camDir;
	float vFovAdjust;

	float3 sunDir;
	float recipOutputSizeX;

	float recipOutputSizeY;
	float aspect;
	float pad1;
	float pad2;
};

struct LightSource
{
	float3 position;
	float radius;
	float3 light;
	float pad0;
};

struct ShadowMapConstants
{
	float3x3 view;

	float3 camPos;
	uint screenRes;

	float3 densityGridOrigin;
	float nearPlane;

	float3 densityTextureScale;
	float farPlane;

	float3 sdfGridOrigin;
	float history;

	float3 sdfTextureScale;
	float offsetX;

	float offsetZ;
	float densityScale;
	float orthoSizeX;
	float orthoSizeY;

	uint samples;
	float time;
};
