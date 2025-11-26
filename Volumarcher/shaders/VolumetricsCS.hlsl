#include "ShaderCommon.h"

static const float PI = 3.14159;

RWTexture2D<float4> outputTexture : register(u0);
Texture2D<float> sceneDepth : register(t0);

cbuffer RootConstants : register(b0)
{
    VolumetricDynamics constants;
};

cbuffer RenderSettings : register(b1)
{
    VolumetricSettings renderConstants;
};

cbuffer WorldSettings : register(b2) {
    VolumetricWorld worldConstants;
}


StructuredBuffer<Volume> volumes : register(t1);

Texture3D<float4> billowNoise : register(t2);
SamplerState noiseSampler : register(s0);
SamplerState profileSampler : register(s1);

Texture3D<float> voxelVolume : register(t3);


static const float FAR_PLANE = 6;

//TODO: Not hardcode this
static const float3 SUN_DIR = normalize(float3(0.4, -1, 0.4));
static const float3 SUN_LIGHT = float3(0.996, 0.9, 0.8) * 10;

static const float3 BACKGROUND_COLOR_UP = float3(0.467, 0.529, 0.671);
static const float3 BACKGROUND_COLOR_DOWN = float3(0.694, 0.596, 0.467) * 0.5;
static const float3 AMBIENT_COLOR = float3(0.467, 0.529, 0.671);


static const float ABSORPTION_SCATTERING = 1.0;
static const float ECCENTRICITY = 0.2;


static const float DEG_TO_RAD = 0.01745;


//Remap from https://stackoverflow.com/a/3451607
float Remap(float value, float low1, float high1, float low2, float high2)
{
    return low2 + (value - low1) * (high2 - low2) / (high1 - low1);
}

//Phase function from https://www.guerrilla-games.com/read/synthesizing-realistic-clouds-for-video-games
float HenyeyGreensteinPhase(float inCosAngle, float inG)
{
    float num = 1.0 - inG * inG;
    float denom = 1.0 + inG * inG - 2.0 * inG * inCosAngle;
    float rsqrt_denom = rsqrt(denom);
    return num * rsqrt_denom * rsqrt_denom * rsqrt_denom * (1.0 / (4.0 * PI));
}

//Sample dimensional profile
float SampleProfile(float3 _sample)
{
    float3 sample = (_sample - renderConstants.origin) * renderConstants.worldSize;
    if (any(sample > 1.0) || any(sample < 0.0))
        return 0.0;
    return voxelVolume.SampleLevel(profileSampler,sample , 0);
}

float SampleDensity(float3 _sample, float _profile)
{
    float density = _profile;
    float scale = 0.4;
	float3 noiseTexSample = float3(_sample) * scale;
    //Wind
    noiseTexSample += worldConstants.wind * constants.time;

	float noise = saturate(billowNoise.SampleLevel(noiseSampler, noiseTexSample, 0).a);
    density = saturate(Remap(density, noise
               , 1, 0, 1));

    return density;
}

float GetSummedAmbientDensity(float3 _sample)
{
    //TODO shaped: Bad step size that assumes 1 sphere
    float stepSize = sqrt(volumes[0].squaredRad) / renderConstants.ambientSampleCount;
    float density = 0;
    for (int i = 0; i < renderConstants.ambientSampleCount; ++i)
    {
        float3 sample = _sample + float3(0, 1, 0) * stepSize * i;
        for (int volumeId = 0; volumeId < VOLUME_AMOUNT; ++volumeId)
        {
            float3 sphereOffset = sample - volumes[volumeId].position;
            float distToSphere2 = dot(sphereOffset, sphereOffset);
            if (distToSphere2 < volumes[volumeId].squaredRad) // Hit sphere
            {
                density += SampleDensity(sample, SampleProfile(sample) * volumes[volumeId].baseDensity) * stepSize;
            }
        }
    }
    return density;
}



float GetDirectLightDensitySamples(float3 _sample)
{

    float totalDensity = 0;
    float stepSize = FAR_PLANE * 0.5 / renderConstants.directLightSampleCount;
    for (int i = 0; i < renderConstants.directLightSampleCount; ++i)
    {
        float3 sample = _sample + -SUN_DIR * (i * stepSize);
        for (int volumeId = 0; volumeId < VOLUME_AMOUNT; ++volumeId)
        {
            float3 sphereOffset = sample - volumes[volumeId].position;
            float distToSphere2 = dot(sphereOffset, sphereOffset);
            if (distToSphere2 < volumes[volumeId].squaredRad) // Hit sphere
            {
                float profile = SampleProfile(sample);
                totalDensity = SampleDensity(sample, profile) * stepSize;
            }
        }
    }
    return totalDensity;
}

float InScatteringApprox(float _baseDimensionalProfile, float _sun_dot, float _sunDensitySamples)
{
    return exp(-_sunDensitySamples * Remap(_sun_dot, 0.0, 0.9, 0.25, Remap(_baseDimensionalProfile, 1.0, 0.0, 0.05, 0.25)));
}



[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x > renderConstants.screenResX || DTid.y > renderConstants.screenResY)
        return;

    float2 screenUV = (float2(DTid.xy) + 0.5) / float2(renderConstants.screenResX, renderConstants.screenResY);

    //Get depth
    float screenDepth = sceneDepth.SampleLevel(noiseSampler, screenUV, 0);
    float linearDepth = renderConstants.zNear * renderConstants.zFar / (renderConstants.zFar + (1 - screenDepth) * (renderConstants.zNear - renderConstants.zFar));
    float farPlane = min(linearDepth, FAR_PLANE);
	
	//Get screen ray
    float aspect = float(renderConstants.screenResX) / float(renderConstants.screenResY);

    float fovAdjust = renderConstants.vFovAdjust;
    float rayX = (2 * screenUV.x - 1) * fovAdjust * aspect;
    float rayY = (1 - 2 * screenUV.y) * fovAdjust;
    //Get camera mat
    float3 camRight = normalize(cross(constants.camDir, float3(0, 1, 0)));
    float3 camUp = cross(camRight, constants.camDir);
    float3x3 camMat = float3x3(camRight, camUp, constants.camDir);

    float3 rayOrigin = constants.camPos;
    float3 rayDir = mul(normalize(float3(rayX, rayY, 1)), camMat);

    //Background (maybe temp or optional if scene already has skybox)
    float3 background = lerp(BACKGROUND_COLOR_DOWN, BACKGROUND_COLOR_UP, saturate((rayDir.y * 0.5) + 0.55));
    if (screenDepth > 0)
        background = outputTexture[DTid.xy];

    static float stepSize = farPlane / renderConstants.baseSampleCount;

    float3 light = 0;
    float transmittance = 1.0;

    //Ray marching steps
    for (int i = 0; i < renderConstants.baseSampleCount; ++i)
    {
        float3 sample = rayOrigin + rayDir * (i * stepSize);
        float sampleDensity = 0;
        float3 sampleLight = 0;

                //Base dimensional profile  (profile goes from 1 - 0     <0 being outside the cloud)
        float profile = SampleProfile(sample);

    	//Density with high detail noise
        sampleDensity += SampleDensity(sample, profile);

                //Ambient approximation, gives popcorn effect
        sampleLight += saturate((1 - profile) * exp(-GetSummedAmbientDensity(sample))) * (AMBIENT_COLOR * PI);
        float lightAngle = dot(rayDir, -SUN_DIR);
        float inSunLightDensitySamples = GetDirectLightDensitySamples(sample);
        float lightVolume = InScatteringApprox(profile, lightAngle, inSunLightDensitySamples);
        sampleLight += lightVolume * SUN_LIGHT * HenyeyGreensteinPhase(lightAngle, ECCENTRICITY);

        light += (sampleLight) * transmittance * sampleDensity * stepSize;

        transmittance *= exp(-stepSize * ABSORPTION_SCATTERING * sampleDensity);
    }
    light += transmittance * background;

    outputTexture[DTid.xy] = float4(light, 1);


}