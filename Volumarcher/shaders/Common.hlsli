static const float PI = 3.14159;

static const float DEG_TO_RAD = 0.01745;

static uint WangHash(uint s)
{
    s = (s ^ 61) ^ (s >> 16);
    s *= 9, s = s ^ (s >> 4);
    s *= 0x27d4eb2d;
    s = s ^ (s >> 15);
    return s;
}

static uint InitSeed(uint seedBase)
{
    return WangHash((seedBase + 1) * 17);
}

static uint RandomUInt(uint customSeed)
{
    customSeed ^= customSeed << 13;
    customSeed ^= customSeed >> 17;
    customSeed ^= customSeed << 5;
    return customSeed;
}
static const float RANDOMUINTTOFLOAT = 2.3283064365387e-10f;



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


float InScatteringApprox(float _baseDimensionalProfile, float _sun_dot, float _sunDensitySamples)
{
    return exp(-_sunDensitySamples * Remap(_sun_dot, 0.0, 0.9, 0.25, Remap(_baseDimensionalProfile, 1.0, 0.0, 0.05, 0.25)));
}

//Code modified from chatGPT code


struct IntersectResult
{
    bool hit;
    float t0;
    float t1;
};
IntersectResult RayBoxIntersection(float3 rayDir, float3 rayOrigin, float3 minBound, float3 maxBound)
{
    // Compute reciprocal of direction to avoid repeated divisions
    float3 invDir = 1.0 / rayDir;

    // t values for each slab
    float3 t0 = (minBound - rayOrigin) * invDir;
    float3 t1 = (maxBound - rayOrigin) * invDir;

    // Order mins and maxs per axis
    float3 tMin3 = min(t0, t1);
    float3 tMax3 = max(t0, t1);

    // Largest entry point
    float tMin = max(max(tMin3.x, tMin3.y), tMin3.z);

    // Smallest exit point
    float tMax = min(min(tMax3.x, tMax3.y), tMax3.z);
    IntersectResult res =
    {

    true, tMin, tMax
        };
    // If the box is missed or behind origin
    if (tMax < 0.0 || tMin > tMax)
    {
        res.hit = false;
        
    }
    return res;
}
