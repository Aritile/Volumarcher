#include "ShaderCommon.h"
#include "Common.hlsli"
#include "atmosphere.hlsli"

#define SDF_ENABLED 1
#define JITTER_SAMPLE 1


RWTexture2D<float4> historyBuffer : register(u0);
Texture2D<float> sceneDepth : register(t0);

cbuffer RootConstants : register(b0)
{
    VolumetricDynamics constants;
};

cbuffer RenderSettings : register(b1)
{
    VolumetricSettings renderConstants;
};

cbuffer WorldSettings : register(b2)
{
    VolumetricWorld worldConstants;
}

StructuredBuffer<LightSource> lights : register(t7);


//Data from lighting cache, R = ambientDensity, G = directLightDensity
Texture3D<float2> cacheData : register(t1);

Texture3D<float2> billowNoise : register(t2);
SamplerState noiseSampler : register(s0);
SamplerState profileSampler : register(s1);
SamplerState blueSampler : register(s2);
SamplerState pointSampler : register(s3);

Texture3D<float> voxelVolumeData : register(t3);
Texture3D<float3> distanceField : register(t4);

Texture2D<float> blueNoise : register(t5);

Texture3D<float4> colorDensityData : register(t6);

//TODO: Not hardcode this
//Rendersettings
static const float TRANSMITTANCE_CUTOFF = 0.01;
//Base values
static const float ADAPTIVESTEP_OVER_DENSITY = 0.03;
//Scaled to samples
static const float adaptiveStepDistance = renderConstants.adaptiveStepOverDistance / float(sqrt(renderConstants.baseSampleCount));
static const float adaptiveStepDensity = ADAPTIVESTEP_OVER_DENSITY / float(sqrt(renderConstants.baseSampleCount));

static const float DIRECTADAPTIVESTEP_OVER_DISTANCE = 0.05;
static const float directAdaptiveStepDistance = DIRECTADAPTIVESTEP_OVER_DISTANCE / float(sqrt(renderConstants.directLightTotalSampleCount));
static const float DIRECTMINSTEP_SCALE = 0.075;

float SampleSDF(float3 _sample)
{
    float3 worldPos = _sample + float3(worldConstants.offsetX, 0, worldConstants.offsetZ);

	float3 sample =
        (worldPos - renderConstants.sdfGridOrigin)
        * renderConstants.sdfTextureScale;

#if 0
    //Non BC sampling
    float rawDistance = min(distanceField.SampleLevel(profileSampler, sample, 0).r, renderConstants.sdfMaxValue);
#else
    float3 BCValue = distanceField.SampleLevel(profileSampler, sample, 0).rgb;
    float decompressed = dot(BCValue, float3(0.96414679, 0.03518212, 0.00067109));
    
	//Raw distance in voxel space
    float rawDistance = (decompressed * 2 - 1) * renderConstants.sdfMaxValue;
#endif
    //Return Distance in world space
    return rawDistance * renderConstants.sdfScale;
}

//Sample dimensional profile
float SampleProfile(float3 _sample)
{
    float3 worldPos = _sample + float3(worldConstants.offsetX, 0, worldConstants.offsetZ);
    int2 sdftile = floor((worldPos.xz - renderConstants.sdfGridOrigin.xz) * renderConstants.sdfTextureScale.xz);
	worldPos.xz += sdftile * (1/renderConstants.densityTextureScale.xz - 1/renderConstants.sdfTextureScale.xz);
    float3 sample = (worldPos - renderConstants.densityGridOrigin) * renderConstants.densityTextureScale;
    float3 rawSample = (_sample - renderConstants.densityGridOrigin) * renderConstants.densityTextureScale;
    if (any(rawSample > 1.0) || any(rawSample < 0.0))
        return 0.0;
	float profile = voxelVolumeData.SampleLevel(profileSampler, sample, 0).r;
    return profile;
}

float UpresProfile(float3 _sample, float _profile, float mip = 0)
{
    float density = _profile;
    float scale = renderConstants.noiseScale;

    float3 noiseTexSample = float3(_sample) * scale;

	//Wind
    noiseTexSample += worldConstants.wind * constants.time;

    float2 noise = saturate(billowNoise.SampleLevel(noiseSampler, noiseTexSample, mip).rg);

    float billowType = pow(_profile, 0.25); // Pow to change the gradient to be more towards high freq,     could be an artistic setting
    float billowNoise = lerp(noise.r * 0.3, noise.g * 0.3, billowType);

    float finalNoise = billowNoise; // TODO (maybe): allow artists to define billow vs wispy

    density = saturate(Remap(density, finalNoise
               , 1.0, 0, 1));
    return density * worldConstants.globalDensityScale;

}



//TODO: This should be precomputed
float GetSummedAmbientDensity(float3 _sample, float _mip)
{
	//Use cache
    float3 worldPos = _sample + float3(worldConstants.offsetX, 0, worldConstants.offsetZ);
    int2 sdftile = floor((worldPos.xz - renderConstants.sdfGridOrigin.xz) * renderConstants.sdfTextureScale.xz);
    worldPos.xz += sdftile * (1 / renderConstants.densityTextureScale.xz - 1 / renderConstants.sdfTextureScale.xz);
	float3 sample = (worldPos - renderConstants.densityGridOrigin) * renderConstants.densityTextureScale;

    float cacheDensity = cacheData.SampleLevel(profileSampler, sample, 0.f).r * renderConstants.ambientScale * 2.0;

    //////Calculate manually
    //float stepSize = (FAR_PLANE * 0.2) / renderConstants.ambientSampleCount;
    //float density = 0;
    //for (int i = 0; i < renderConstants.ambientSampleCount; ++i)
    //{
    //    float3 sample = _sample + float3(0, 1, 0) * stepSize * i;

    //    density += UpresProfile(sample, SampleProfile(sample), _mip) * stepSize;
    //}
    return (cacheDensity);
}

float2 WorldToScreen(float3 _world, float3 _camDir, float3 _camPos, float _fovAdjust, float _aspect)
{
    float3 view = _world - _camPos;

    float3 camRight = normalize(cross(_camDir, float3(0, 1, 0)));
    float3 camUp = normalize(cross(camRight, _camDir));

    float x = dot(view, camRight);
    float y = dot(view, camUp);
    float z = dot(view, _camDir);

    float ndcX = (x / z) / (_fovAdjust * _aspect);
    float ndcY = (y / z) / (_fovAdjust);
    if (z <= 0.0)
        return float2(-1, -1);
    float2 uv;
    uv.x = ndcX * 0.5 + 0.5;
    uv.y = 0.5 - ndcY * 0.5;

    return uv;
}

float GetDirectLightDensitySamples(float3 _sample, float _mip)
{


    ////Full real samples
    //float totalDensityReal = 0;
    //int samples = renderConstants.directLightTotalSampleCount - renderConstants.directLightRealSampleCount;
    //float stepSizeReal = (renderConstants.volumetricFarPlane / (samples));
    //float3 sampleReal = _sample;
    //for (int i = 0; i < samples; ++i)
    //{
    //    sampleReal += -worldConstants.sunDir * (stepSizeReal);
    //    float profile = SampleProfile(sampleReal);
    //    if (profile <= 0.0)
    //    {
    //        stepSizeReal += directAdaptiveStepDistance * stepSizeReal;
    //        continue;
    //    }
    //    totalDensityReal += UpresProfile(sampleReal, profile, _mip) * stepSizeReal;
    //    stepSizeReal += directAdaptiveStepDistance * stepSizeReal;
    //}

    //Use combination of first steps real then cache
    float totalDensity = 0;
    float stepSize = (renderConstants.directLightingFarPlane / (renderConstants.directLightTotalSampleCount));
    float3 sample = _sample;
    for (int i = 0; i < renderConstants.directLightRealSampleCount; ++i)
    {
        sample += -worldConstants.sunDir * (stepSize);
        float profile = SampleProfile(sample);
        if (profile <= 0.0)
        {
            stepSize += directAdaptiveStepDistance * stepSize;
            continue;
        }
        totalDensity += UpresProfile(sample, profile, _mip) * stepSize;
        stepSize += directAdaptiveStepDistance * stepSize;
    }
    if (renderConstants.directLightRealSampleCount < renderConstants.directLightTotalSampleCount)
    {

    //Cached data
        float3 worldPos = sample + float3(worldConstants.offsetX, 0, worldConstants.offsetZ);
        int2 sdftile = floor((worldPos.xz - renderConstants.sdfGridOrigin.xz) * renderConstants.sdfTextureScale.xz);
        worldPos.xz += sdftile * (1 / renderConstants.densityTextureScale.xz - 1 / renderConstants.sdfTextureScale.xz);
        float3 oSample = (worldPos - renderConstants.densityGridOrigin) * renderConstants.densityTextureScale;

        float cachedLighting = cacheData.SampleLevel(profileSampler,oSample , 0.f).g * renderConstants.ambientScale * worldConstants.globalDensityScale;
        totalDensity += cachedLighting;
    }

    return totalDensity;
}

float3 SampleLocalLight(float3 _sample, float _density)
{
    float3 light = 0;
    for (int i = 0; i <= constants.amountOfLights; ++i)
    {
        float d1 = length(lights[i].position - _sample);
        float rad = 1 - (d1 / lights[i].radius);
        if (rad > 0)
        {
            float potential = pow(rad, 12.0);
            float attentuation = (1.0 - saturate(_density));
            light += potential * attentuation * lights[i].light;
        }
    }
    return light;
}


[numthreads(VOLUMETRIC_PASS_GROUP_SIZE, VOLUMETRIC_PASS_GROUP_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x > renderConstants.screenResX || DTid.y > renderConstants.screenResY)
        return;

    float2 screenUV = (float2(DTid.xy) + 0.5) / float2(renderConstants.screenResX, renderConstants.screenResY);

    //Get depth
    float screenDepth = sceneDepth.SampleLevel(profileSampler, screenUV, 0);

    //Avoid halo on lower resolution
    if (renderConstants.resolutionScale > 1.0)
    {
        float2 texelSize = 1 / (float2(renderConstants.screenResX, renderConstants.screenResY) * renderConstants.resolutionScale);
        float2 offsets[4] =
        {
            float2(-1, -1),
		float2(1, -1),
		float2(-1, 1),
		float2(1, 1)
        };

        for (int i = 0; i < 4; i++)
        {
            float d = sceneDepth.SampleLevel(
        profileSampler,
        screenUV + offsets[i] * texelSize,
        0);
            screenDepth = min(screenDepth, d);
        }
    }


    float linearDepth =
    (renderConstants.zNear * renderConstants.zFar) /
    (renderConstants.zNear + screenDepth * (renderConstants.zFar - renderConstants.zNear));

	//float linearDepth = renderConstants.zNear * renderConstants.zFar / (renderConstants.zFar + (1 - screenDepth) * (renderConstants.zNear - renderConstants.zFar));
    if (linearDepth >= renderConstants.zFar)
        linearDepth = 10e99;
    float farPlane = renderConstants.targetRenderDistance;
	
	//Get screen ray
    float aspect = float(renderConstants.screenResX) / float(renderConstants.screenResY);

    float fovAdjust = renderConstants.vFovAdjust;
    float rayX = (2 * screenUV.x - 1) * fovAdjust * aspect;
    float rayY = (1 - 2 * screenUV.y) * fovAdjust;
    //Get camera mat
    float3 camRight = normalize(cross(constants.camDir, float3(0, 1, 0)));
    float3 camUp = normalize(cross(camRight, constants.camDir));
    float3x3 camMat = float3x3(camRight, camUp, constants.camDir);

    float3 rayOrigin = constants.camPos;
    float3 rayDir = mul(normalize(float3(rayX, rayY, 1)), camMat);

	//Set to volume
    float3 minBound = renderConstants.densityGridOrigin;
    float3 maxBound = minBound + 1 / (renderConstants.densityTextureScale);
	
    IntersectResult t0t1 = RayBoxIntersection(rayDir, rayOrigin, minBound, maxBound);


    float t0viewZ = t0t1.t0 * dot(rayDir, constants.camDir);

    int sampleCount = renderConstants.baseSampleCount;
    if (t0t1.hit == false || (t0viewZ >= linearDepth && linearDepth < renderConstants.zFar)) //TODO: possibly do a mask pass before to avoid huge branch
    {
        historyBuffer[DTid.xy] = float4(0, 0, 0, 9999); // transmission of > 1 == invalid pixel
        return;
    }
    float3 samplePosition = rayOrigin;
    if (t0t1.t0 >= 0)
    {
        samplePosition += rayDir * t0t1.t0;
        linearDepth -= t0viewZ;
    }
    float t1viewZ = t0t1.t1 * dot(rayDir, constants.camDir);
	
    linearDepth = min(t1viewZ, linearDepth);

    //float stepSize = farPlane / renderConstants.baseSampleCount;
    float stepSize = farPlane / (sampleCount);
#if SDF_ENABLED == 1
    float MIN_SDF = (farPlane / (sampleCount));
    float adaptiveSize = MIN_SDF;
#endif
    float3 light = 0;
    float totalDensity = 0;

#if JITTER_SAMPLE == 1
    uint w, h, m;
    blueNoise.GetDimensions(0, w, h, m);
    float seed = blueNoise.SampleLevel(blueSampler, ((DTid.xy + 0.5) / float2(w, h)), 0);
    uint randSeed = InitSeed((uint) (seed * 256.0));
#endif
    float dist = 0;

    //Loop constant variables
    float lightAngle = dot(rayDir, -worldConstants.sunDir);
    float phase = HenyeyGreensteinPhase(lightAngle, worldConstants.cloudEccentricity);

    float3 hit = float3(0.f, 0.f, 0.f);



    //Ray marching steps
    for (int i = 0; i < sampleCount; ++i)
    {

    	//Clamp to rasterisation
        float viewZ = dist * dot(rayDir, constants.camDir);

        if (viewZ >= linearDepth)
            break;
        if (viewZ + stepSize > linearDepth)
            stepSize = linearDepth - viewZ;



    	dist += stepSize;
#if SDF_ENABLED == 1
        adaptiveSize = (sqrt(dist) * adaptiveStepDistance) + (adaptiveStepDensity * totalDensity);
#endif

#if JITTER_SAMPLE == 1
        randSeed = RandomUInt(randSeed);
        float3 sample = samplePosition + rayDir * stepSize * (randSeed * RANDOMUINTTOFLOAT);
#endif

        samplePosition += rayDir * stepSize;

#if JITTER_SAMPLE != 1
    	float3 sample = samplePosition;
#endif
        float3 sampleLight = 0;

        //Check if there is an SDF texture
        if (renderConstants.sdfScale != 0)
        {

            float distanceField = SampleSDF(samplePosition);


            if (distanceField > 0)
            {
#if SDF_ENABLED == 1
                stepSize = max(distanceField, adaptiveSize);
#endif
                continue;
            }
        }

        //hit position for reprojection, use non jittered to be more consistant
        if (totalDensity <= 0.1)
            hit = samplePosition;

        //Base dimensional profile  (profile goes from 1 - 0     <0 being outside the cloud)
        float profile = SampleProfile(sample);
        float mip = log2(1.0 + (dist * 50.0)); // TODO: Mip bias as var
    	//Density with high detail noise
        float sampleDensity = UpresProfile(sample, profile, mip);

       //Early continue if no contribution
        if (sampleDensity <= 0)
        {
#if SDF_ENABLED == 1
            stepSize = adaptiveSize;
#endif
            continue;
        }

		
        
        //Ambient approximation, gives popcorn effect
        sampleLight += saturate(pow(profile, 0.5) * exp(-GetSummedAmbientDensity(sample, mip))) * (worldConstants.ambientLight);

    	//Direct light
        float inSunLightDensitySamples = GetDirectLightDensitySamples(sample, mip);

#if 1
       //Multiple scattering approx
        float lightVolume = InScatteringApprox(1 - profile, lightAngle, inSunLightDensitySamples);
#else
    	//Single scattering
        float lightVolume = exp(-inSunLightDensitySamples);
#endif
        sampleLight += lightVolume * worldConstants.sunLight * phase;

		//Local light
        sampleLight += SampleLocalLight(sample, sampleDensity);

        totalDensity += sampleDensity * stepSize;
        float transmittance = exp(-totalDensity);
        float3 cloudColor = worldConstants.cloudAbsorptionColor;
        if (renderConstants.hasColorDensityGrid == 1)
        {
            float3 worldPos = sample + float3(worldConstants.offsetX, 0, worldConstants.offsetZ);
            int2 sdftile = floor((worldPos.xz - renderConstants.sdfGridOrigin.xz) * renderConstants.sdfTextureScale.xz);
            worldPos.xz += sdftile * (1 / renderConstants.densityTextureScale.xz - 1 / renderConstants.sdfTextureScale.xz);
            float3 colorSample = (worldPos - renderConstants.densityGridOrigin) * renderConstants.densityTextureScale;
            cloudColor *= colorDensityData.SampleLevel(profileSampler, colorSample, 0).rgb;
        }
        light += sampleLight * transmittance * stepSize * sampleDensity * cloudColor;
		
        if (transmittance < TRANSMITTANCE_CUTOFF)
        {
            break;
        }
#if SDF_ENABLED == 1
        stepSize = adaptiveSize;
#endif
    }
    float transmittance = exp(-totalDensity);
    if (transmittance <= TRANSMITTANCE_CUTOFF)
        transmittance = 0;

    float4 result = float4(light.rgb, transmittance);
    if (any(hit > 0) &&
        renderConstants.reprojectAmount > 0.0)
    {
        //hit /= totalDensity;
        float2 historyUV = (WorldToScreen(hit, constants.oldCamDir, constants.oldCamPos, renderConstants.vFovAdjust,
        renderConstants.screenResX / float(renderConstants.screenResY))) * (float2(renderConstants.screenResX, renderConstants.screenResY) - 1.0);


        if (all(historyUV > 0) && historyUV.x < renderConstants.screenResX && historyUV.y < renderConstants.screenResY)
        {
            uint2 pixel = round(historyUV);
            float4 history = historyBuffer[pixel];
            if (history.a > 1.0) // Count transmission > 1 as invalid pixels
            {
                result = float4(light.rgb, transmittance);
                
            }
            else
            {

                float3 posOffset = constants.oldCamPos - constants.camPos;
                float dirOffset = 1 - dot(constants.oldCamDir, constants.camDir);
                float posInaccuracy = max((dot(posOffset, posOffset) - 0.000001), 0) * 500;
                float dirInaccuracy = dirOffset * 40000;

                float offset = dot(hit - constants.camPos, constants.camDir);
                float distanceInaccuracy = max(5 / max(offset, 3) - 0.7, 0);
                //historyBuffer[DTid.xy] = float4(distanceInaccuracy, 0, 0, 1);
                //return;
                float inaccuracy = posInaccuracy + dirInaccuracy + distanceInaccuracy;

                float d = max(abs(screenUV * 2.0 - 1.0).x,
              abs(screenUV * 2.0 - 1.0).y);

                float t = clamp((d - 0.6) / 0.4, 0.0, 1.0);
                float edge = pow(t, 2);

                inaccuracy += (edge * 2);

                inaccuracy *= 1 + (renderConstants.resolutionScale - 1) * 1;

                inaccuracy += dot(light.rgb, light.rgb) * 0.15; // prefer light

                inaccuracy = max(inaccuracy, 0);

                result = lerp(float4(light.rgb, transmittance), history, (0.97f * renderConstants.reprojectAmount) / (1 + inaccuracy));
            }
        }
        else
        {
            result = float4(light.rgb, transmittance);

        }
    }


    historyBuffer[DTid.xy] = result;
}