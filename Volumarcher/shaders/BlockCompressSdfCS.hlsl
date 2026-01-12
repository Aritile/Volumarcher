

#include "BlockCompressionFunctions.hlsli"
#include "ShaderCommon.h"

RWStructuredBuffer<uint2> outBuffer;
Texture3D<float> sourceTex;

//Linear sampler
SamplerState sourceSampler : register(s0);

cbuffer RootConstants : register(b0)
{
    CompressConstants constants;
};



[numthreads(COMPRESSION_GROUP_SIZE, COMPRESSION_GROUP_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (any(DTid >= constants.outSize))
        return;

    float3 sizeRecip = 1.0 / float3(constants.outSize);
    float3 uv = (DTid + 0.5) * sizeRecip;
    

    float data[16];
    for (int i = 0; i < 16; ++i)
    {
        int x = (i & 3);
        int y = (i >> 2);
        float sX = x * sizeRecip.x*0.25; // outSize is in blocks so /4
        float sY = y * sizeRecip.y*0.25;
        float sourcePixel = sourceTex.SampleLevel(sourceSampler, float3(uv.x + sX, uv.y + sY, uv.z), 0);
        float normalized = sourcePixel * constants.sdfMaxSizeReciprical * 0.5 + 0.5;
        data[i] = normalized;
    }

    uint2 bcBlock;
    sBuildBC1Block(data, float3(0.96414679, 0.03518212, 0.00067109), bcBlock);

    outBuffer[DTid.z * constants.outSize.x * constants.outSize.y + DTid.y * constants.outSize.x + DTid.x] = bcBlock;
}