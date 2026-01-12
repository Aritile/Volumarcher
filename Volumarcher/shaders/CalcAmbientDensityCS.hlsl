#include "ShaderCommon.h"
RWTexture3D<float2> output : register(u0);

Texture3D<float> voxelVolumeData : register(t0);
Texture3D<float> noiseData : register(t1);
SamplerState profileSampler : register(s0);


cbuffer constants : register(b0)
{
    AmbientPreprocessSettings constants;
};

static const float ADAPTIVESTEP_OVER_DISTANCE = 0.0001;
static const float MINSTEP_SCALE = 0.5;

//Sample dimensional profile
float SampleProfile(float3 _sample)
{
    float3 sample = _sample / float3(constants.outputSize);
    if (any(sample > 1.0) || any(sample < 0.0))
        return 0.0;
    float profile = voxelVolumeData.SampleLevel(profileSampler, sample, 0).r;
    return profile;
}

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (any(float3(DTid) > constants.outputSize))
        return;

    float3 samplePos = float3(DTid+0.5);


    float stepSize = ((constants.outputSize.y) / float(constants.sampleCount)); // * MINSTEP_SCALE;
    float totalDensity = 0.f;
    for (int i = 0; i < constants.sampleCount; ++i)
    {
        totalDensity += SampleProfile(samplePos)*stepSize;
        samplePos.y += stepSize;

        //stepSize += ADAPTIVESTEP_OVER_DISTANCE * stepSize;
    }
    //Since we calculated in outputSize space, scale by gridsize to get them in input grid space
    totalDensity /= constants.gridScale;

    output[DTid] = float2(totalDensity,0.0);
}