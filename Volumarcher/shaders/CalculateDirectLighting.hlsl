#include "Common.hlsli"
#include "shaderCommon.h"

RWTexture3D<float2> output : register(u0);

Texture3D<float> densityVoxels : register(t0);
Texture3D<float> sdfVoxels : register(t1);
Texture3D<float4> noiseData : register(t2);
SamplerState profileSampler : register(s0);
SamplerState noiseSampler : register(s1);



cbuffer constants : register(b0)
{
    DirectLightingSettings constants;
};
 

//Sample dimensional profile
float SampleProfile(float3 _sample)
{
    float3 sample = _sample / float3(constants.outputSize);
    if (any(sample > 1.0) || any(sample < 0.0))
        return 0.0;
    float profile = densityVoxels.SampleLevel(profileSampler, sample, 0).r;
    return profile;
}

float UpresProfile(float3 _sample, float _profile, float mip = 0)
{
    float density = _profile;
    float scale = constants.noiseScale;
    float3 noiseTexSample = float3(_sample) / float3(constants.outputSize) * scale;
    //Wind
    noiseTexSample += constants.wind * constants.time;

    float4 noise = saturate(noiseData.SampleLevel(noiseSampler, noiseTexSample, mip));

    float billowType = pow(_profile, 0.25); // Pow to change the gradient to be more towards high freq,     could be an artistic setting
    float billowNoise = lerp(noise.b * 0.3, noise.a * 0.3, billowType);

    float finalNoise = billowNoise; // TODO (maybe): allow artists to define billow vs wispy

    density = saturate(Remap(density, finalNoise
               , 1, 0, 1));
    return density;

}



[numthreads(8, 8, 8)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint3 dispatchPos = DTid + constants.dispatchOffset;
    if (any(dispatchPos > constants.outputSize))
        return;

    //TODO: Update only part
    float3 samplePos = float3(dispatchPos + 1.0);
    float stepSize = ((constants.farPlane) / float(constants.sampleCount)) * constants.gridScale;
    float totalDensity = 0;

    uint randSeed = InitSeed((dispatchPos.x + dispatchPos.y * dispatchPos.x)*constants.time);

    for (int i = 0; i < constants.sampleCount; ++i)
    {
        randSeed = RandomUInt(randSeed);
        float3 sample = samplePos + -constants.sunDir * stepSize * (randSeed * RANDOMUINTTOFLOAT);
        totalDensity += UpresProfile(sample, SampleProfile(sample)) * stepSize;

        samplePos += -constants.sunDir * stepSize;
    }
    totalDensity /= constants.gridScale;
    output[dispatchPos].g = lerp(output[dispatchPos].g, totalDensity, 0.1);
}