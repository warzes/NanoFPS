#define IS_SHADER
#include "Common.hlsli"
#include "TransparencyVS.hlsli"

float4 psmain(VSOutput input) : SV_TARGET
{
    return float4(input.color * g_Globals.meshOpacity, g_Globals.meshOpacity);
}
