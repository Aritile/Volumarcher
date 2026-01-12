//Athmosphere based on https://www.shadertoy.com/view/Ml2cWG


static const float pi = 3.14159265359;
static const float invPi = 1.0 / pi;
static const float zenithOffset = 0.0;
static const float multiScatterPhase = 0.1;
static const float density = 0.7;
static const float anisotropicIntensity = 1.0; //Higher numbers result in more anisotropic scattering
static const float3 skyColorDay = float3(0.39, 0.57, 1.0) * (1.0 + anisotropicIntensity); //Make sure one of the conponents is never 0.0

static const float3 skyColorNight = float3(0.98, 0.98, 0.8) * (1.0 + anisotropicIntensity); //Make sure one of the conponents is never 0.0

#define smooth(x) x*x*(3.0-2.0*x)
#define zenithDensity(x) density / pow(max(x - zenithOffset, 0.35e-2), 0.75)

float3 getSkyAbsorption(float3 x, float y)
{
	
    float3 absorption = x * -y;
    absorption = exp2(absorption) * 2.0;
	
    return absorption;
}

float getSunPoint(float3 rayDir, float3 sunDir)
{
    float cosAngle = saturate(dot(rayDir, sunDir));
    return smoothstep(0.9995, 1.0, cosAngle) * 50.0;
}

float getRayleigMultiplier(float3 rayDir, float3 sunDir)
{
    float cosAngle = saturate(dot(rayDir, sunDir));
    return 1.0 + pow(1.0 - cosAngle, 2.0) * 3.14159 * 0.5;
}

float getMie(float3 rayDir, float3 sunDir)
{
    float cosA = saturate(dot(rayDir, sunDir));
    float angle = acos(cosA); // angular distance in radians
    float norm = angle / 3.14159265; // remap to 0–1 (just like distance(p,lp))

    float disk = clamp(1.0 - pow(norm, 0.1), 0.0, 1.0);

    return disk * disk * (3.0 - 2.0 * disk) * 2.0 * pi;
}

float3 jodieReinhardTonemap(float3 c)
{
    float l = dot(c, float3(0.2126, 0.7152, 0.0722));
    float3 tc = c / (c + 1.0);

    return lerp(c / (l + 1.0), tc, tc);
}

//ChatGPT star generation code
//--------
float hash31(float3 p)
{
    p = frac(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return frac((p.x + p.y) * p.z);
}

#define STAR_DENSITY 300.0

float getStars(float3 rayDir)
{
    // Project direction into cube map index space
    float3 p = normalize(rayDir) * STAR_DENSITY;

    // Use cell position — not spherical projection
    float3 cell = floor(p);

    // hash to determine brightness
    float h = hash31(cell);

    // threshold -> star or no star
    float star = step(0.995, h); // 0.5% of cells become stars

    // brightness (tiny twinkling variation)
    float b = h * star;

    // tiny soft falloff
    return b * b * saturate(rayDir.y) * 3;
}
//--------
float3 getAtmosphericScattering(float3 rayDir, float3 sunDir, float sunStrength = 1.0)
{
    float night = 1-saturate((sunDir.y + 0.1) * 7);

    float zenith = zenithDensity(rayDir.y+(0.1*night));
    float sunPointDistMult = clamp(length(max(sunDir.y + multiScatterPhase - zenithOffset, 0.0)), 0.0, 1.0);

	float3 skyColor = lerp(skyColorDay, skyColorNight, night);

    float rayleighMult = getRayleigMultiplier(rayDir, -sunDir);
	
    float3 absorption = getSkyAbsorption(skyColor, zenith);
    float3 sunAbsorption = getSkyAbsorption(skyColor, zenithDensity(sunDir.y + multiScatterPhase));
    float3 sky = skyColor * zenith * rayleighMult;
    float3 sun = getSunPoint(rayDir, sunDir) * absorption;
    float3 mie = getMie(rayDir, sunDir) * sunAbsorption;
	
    float3 totalSky = lerp(sky * absorption, sky / (sky + 0.5), sunPointDistMult);
    totalSky += (sun + mie) * sunStrength;
    totalSky *= sunAbsorption * 0.5 + 0.5 * length(sunAbsorption);

    //Night
    totalSky = lerp(totalSky, absorption*0.2, night) + getStars(rayDir)*night*sunStrength;

    return totalSky*0.1;
}
