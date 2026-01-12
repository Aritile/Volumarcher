#include "shaderCommon.h"
#include "atmosphere.hlsli"

RWTexture2D<float4> output : register(u0);

Texture2D<float> depthInput : register(t0);

SamplerState texSampler : register(s0);

cbuffer constants : register(b0)
{
    AtmosphereRenderConstants constants;
}


[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float2 uv = float2(DTid.xy + 0.5) * float2(constants.recipOutputSizeX,constants.recipOutputSizeY);

    float depth = depthInput.SampleLevel(texSampler, uv, 0).x;

    float4 color;
    if (depth > 0)
    {
        color = output[DTid.xy].rgba;

    }else
    {
		//TODO: (maybe) add option to mask cloud pass here for more performance if clouds are always behind

		//Get screen ray
        float aspect = constants.aspect;

        float fovAdjust = constants.vFovAdjust;
        float rayX = (2 * uv.x - 1) * fovAdjust * aspect;
        float rayY = (1 - 2 * uv.y) * fovAdjust;
		//Get camera mat
        float3 camRight = normalize(cross(constants.camDir, float3(0, 1, 0)));
        float3 camUp = cross(camRight, constants.camDir);
        float3x3 camMat = float3x3(camRight, camUp, constants.camDir);

        float3 rayDir = mul(normalize(float3(rayX, rayY, 1)), camMat);
        color = float4(getAtmosphericScattering(rayDir, -constants.sunDir).rgb, 1.f);
	    
    }

    output[DTid.xy] = color;

}