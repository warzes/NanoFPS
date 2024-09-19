#define IS_SHADER
#include "Common.hlsli"
#include "FullscreenVS.hlsli"

SamplerState NearestSampler      : register(CUSTOM_SAMPLER_0_REGISTER);
Texture2D    OpaqueTexture       : register(CUSTOM_TEXTURE_0_REGISTER);
Texture2D    TransparencyTexture : register(CUSTOM_TEXTURE_1_REGISTER);

float4 psmain(VSOutput input) : SV_TARGET
{
    const float3 opaqueColor = OpaqueTexture.Sample(NearestSampler, input.uv).rgb;

    const float4 transparencySample = TransparencyTexture.Sample(NearestSampler, input.uv);
    const float3 transparencyColor  = transparencySample.rgb;
    const float  coverage           = transparencySample.a;

    const float3 finalColor = transparencyColor + ((1.0f - coverage) * opaqueColor);
    return float4(finalColor, 1.0f);
}
