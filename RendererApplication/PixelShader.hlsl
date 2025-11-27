struct Input
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL0;
};

static const float3 SUN_DIR = normalize(float3(-0.5, -1, -0.5));
static const float3 SUN_LIGHT = float3(0.996, 0.85, 0.7) * 10;

static const float3 BACKGROUND_COLOR_UP = float3(0.167, 0.229, 0.971);
static const float3 BACKGROUND_COLOR_DOWN = float3(0.467, 0.529, 0.971);

float4 main(Input input) : SV_TARGET
{
    float NdotL = saturate(dot(input.normal, -SUN_DIR)); 

    float3 diffuse = SUN_LIGHT * NdotL;

    float h = saturate((input.normal.y * 0.5) + 0.55);
    float3 ambient = lerp(BACKGROUND_COLOR_DOWN, BACKGROUND_COLOR_UP, h)*3.14;

    float3 color = (diffuse + ambient) * float3(0.85, 0.85, 0.85);

    return float4(color, 1.0);
}