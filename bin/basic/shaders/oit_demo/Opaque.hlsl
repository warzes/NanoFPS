#define IS_SHADER
#include "Common.hlsli"

struct VSOutput
{
    float4 position : SV_POSITION;
};

VSOutput vsmain(float4 position : POSITION)
{
    VSOutput result;
    result.position = mul(g_Globals.backgroundMVP, position);
    return result;
}

float4 psmain(VSOutput input) : SV_TARGET
{
    return g_Globals.backgroundColor;
}
