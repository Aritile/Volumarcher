#include "shaderCommon.h"

RWTexture2D<float4> output : register(u0);
Texture2D<float4> history : register(t0);

SamplerState texSampler : register(s0);

cbuffer RootConstants : register(b0)
{
    BlitValues constants;
};


[numthreads(COPY_GROUP_SIZE, COPY_GROUP_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x > constants.outputX || DTid.y > constants.outputX)
        return;

    float2 uv = (DTid.xy+0.5) / float2(constants.outputX, constants.outputY);

    float4 historySample = history.SampleLevel(texSampler, uv, 0);
    float3 background = output[DTid.xy].rgb;

    float3 result;

    if (historySample.a <= 1.0)
    {
        result = historySample.rgb + background * historySample.a;
    }else
    {
        result = background;
    }

    output[DTid.xy] = float4(result, 1.0);
}