
#include "Common.hlsli"
#include "ShaderCommon.h"


RWTexture2D<float> outputShadow : register(u0);
Texture2D<float> inputDepth : register(t0);

Texture3D<float> voxelVolumeData : register(t1);

SamplerState profileSampler : register(s0);


cbuffer Constants : register(b0)
{
    ShadowMapConstants constants;
}

static const float TRANSMITTANCE_CUTOFF = 0.01;


//Sample dimensional profile
float SampleProfile(float3 _sample)
{
    float3 worldPos = _sample + float3(constants.offsetX, 0, constants.offsetZ);
    int2 sdftile = floor((worldPos.xz - constants.sdfGridOrigin.xz) * constants.sdfTextureScale.xz);
    worldPos.xz += sdftile * (1 / constants.densityTextureScale.xz - 1 / constants.sdfTextureScale.xz);
    float3 sample = (worldPos - constants.densityGridOrigin) * constants.densityTextureScale;
    float3 rawSample = (_sample - constants.densityGridOrigin) * constants.densityTextureScale;
    if (any(rawSample > 1.0) || any(rawSample < 0.0))
        return 0.0;
    float profile = voxelVolumeData.SampleLevel(profileSampler, sample, 0).r;
    return profile;
}

[numthreads(SHADOW_PASS_GROUP_SIZE, SHADOW_PASS_GROUP_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (any(DTid > constants.screenRes))
        return;

    float2 screenUV = (float2(DTid.xy) + 0.5) / float2(constants.screenRes, constants.screenRes);

    float2 ndc = screenUV * 2 - 1;
    ndc.y = -ndc.y;

// Extract basis from view matrix (world space)
    float3x3 view = constants.view;
    float3 right = float3(view._11, view._12, view._13);
    float3 up = float3(view._21, view._22, view._23);
    float3 forward = float3(view._31, view._32, view._33);

    float3 rayDir = normalize(forward);

    float3 rayOrigin = constants.camPos +
    ndc.x * constants.orthoSizeX * right +
    ndc.y * constants.orthoSizeY * up;
    
	//Set to volume
    float3 minBound = constants.densityGridOrigin;
    float3 maxBound = minBound + 1 / (constants.densityTextureScale);
	
    IntersectResult t0t1 = RayBoxIntersection(rayDir, rayOrigin, minBound, maxBound);

    int sampleCount = constants.samples;
    if (t0t1.hit == false )
    {
        outputShadow[DTid.xy] = 1.f;
        return;
    }
    float3 samplePosition = rayOrigin;
    if (t0t1.t0 >= 0)
    {
        samplePosition += rayDir * t0t1.t0;
        //linearDepth -= t0viewZ;
    }

    float stepSize = (t0t1.t1 - max(t0t1.t0,0.0)) / (sampleCount);

    float totalDensity = 0;

    uint seed = (DTid.x + DTid.y * constants.screenRes) * (constants.time*16);
    uint randSeed = InitSeed(seed);
    float dist = 0;

    //Ray marching steps
    for (int i = 0; i < sampleCount; ++i)
    {
		//Clamp
        if (dist >= t0t1.t1)
            break;
        if (dist + stepSize > t0t1.t1)
            stepSize = t0t1.t1 - dist;

        dist += stepSize;

        randSeed = RandomUInt(randSeed);
        float3 sample = samplePosition + rayDir * stepSize * (randSeed * RANDOMUINTTOFLOAT);

        samplePosition += rayDir * stepSize;

        //Base dimensional profile  (profile goes from 1 - 0     <0 being outside the cloud)
        float profile = SampleProfile(sample);

    	//Density with high detail noise
        float sampleDensity = profile * constants.densityScale;

        totalDensity += sampleDensity * stepSize;
        float transmittance = exp(-totalDensity);
		
        if (transmittance < TRANSMITTANCE_CUTOFF)
        {
            break;
        }
    }
    float transmittance = exp(-totalDensity);
    if (transmittance <= TRANSMITTANCE_CUTOFF)
        transmittance = 0;

    //float history = outputShadow[DTid.xy];
    outputShadow[DTid.xy] = transmittance;
}