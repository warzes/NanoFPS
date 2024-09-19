#define IS_SHADER
#include "Common.hlsli"
#include "TransparencyVS.hlsli"

struct PSOutput
{
    float4 color    : SV_TARGET0;
#if defined(WEIGHTED_AVERAGE_FRAGMENT_COUNT)
	float  count    : SV_TARGET1;
#elif defined(WEIGHTED_AVERAGE_EXACT_COVERAGE)
    float  coverage : SV_TARGET1;
#else
#error
#endif
};

PSOutput psmain(VSOutput input)
{
    PSOutput output = (PSOutput)0;
    output.color    = float4(input.color * g_Globals.meshOpacity, g_Globals.meshOpacity);
#if defined(WEIGHTED_AVERAGE_FRAGMENT_COUNT)
    output.count    = 1.0f;
#elif defined(WEIGHTED_AVERAGE_EXACT_COVERAGE)
    output.coverage = (1.0f - g_Globals.meshOpacity);
#else
#error
#endif
    return output;
}
