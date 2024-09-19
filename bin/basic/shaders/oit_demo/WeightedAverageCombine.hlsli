#define IS_SHADER
#include "Common.hlsli"
#include "FullscreenVS.hlsli"

SamplerState     NearestSampler  : register(CUSTOM_SAMPLER_0_REGISTER);
Texture2D        ColorTexture    : register(CUSTOM_TEXTURE_0_REGISTER);
#if defined(WEIGHTED_AVERAGE_FRAGMENT_COUNT)
Texture2D<float> CountTexture    : register(CUSTOM_TEXTURE_1_REGISTER);
#elif defined(WEIGHTED_AVERAGE_EXACT_COVERAGE)
Texture2D<float> CoverageTexture : register(CUSTOM_TEXTURE_1_REGISTER);
#else
#error
#endif

float4 psmain(VSOutput input) : SV_TARGET
{
    const float4 colorInfo    = ColorTexture.Sample(NearestSampler, input.uv);
    const float3 colorSum     = colorInfo.rgb;
    const float  alphaSum     = max(colorInfo.a, EPSILON);
    const float3 averageColor = colorSum / alphaSum;

#if defined(WEIGHTED_AVERAGE_FRAGMENT_COUNT)
    const uint count     = max(1, (uint)CountTexture.Sample(NearestSampler, input.uv));
    const float coverage = 1.0f - pow(max(0.0f, 1.0f - (alphaSum / count)), count);
#elif defined(WEIGHTED_AVERAGE_EXACT_COVERAGE)
    const float coverage = 1.0f - CoverageTexture.Sample(NearestSampler, input.uv);
#else
#error
#endif

    return float4(averageColor * coverage, coverage);
}
