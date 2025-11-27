#include "ShaderCommon.h"
RWTexture3D<float4> outMip1;
Texture3D<float4> sourceMip;

//Linear sampler
SamplerState sourceSampler : register(s0);

cbuffer RootConstants : register(b0)
{
    MipConstants constants;
};

//TODO: Multiple per dispatch using groupshared

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float3 uv = (float3(DTid) + 0.5) / float(constants.outSize); // UV offset to corner of the pixel to sample neighbours

    float4 sample = sourceMip.SampleLevel(sourceSampler, uv, 0);

    outMip1[DTid.xyz] = sample;
}