#define IS_SHADER
#include "Common.hlsli"
#include "TransparencyVS.hlsli"

SamplerState NearestSampler     : register(CUSTOM_SAMPLER_0_REGISTER);
Texture2D    OpaqueDepthTexture : register(CUSTOM_TEXTURE_0_REGISTER);
#if !defined(IS_FIRST_LAYER)
Texture2D    LayerDepthTexture  : register(CUSTOM_TEXTURE_1_REGISTER);
#endif

float4 psmain(VSOutput input) : SV_TARGET
{
    float2 textureDimension = (float2)0;
    OpaqueDepthTexture.GetDimensions(textureDimension.x, textureDimension.y);
    const float2 uv = input.position.xy / textureDimension;

    // Test against opaque depth
    {
        const float opaqueDepth = OpaqueDepthTexture.Sample(NearestSampler, uv).r;
        clip(input.position.z < opaqueDepth ? 1.0f : -1.0f);
    }
#if !defined(IS_FIRST_LAYER)
    // Test against previous layer depth
    {
        const float layerDepth = LayerDepthTexture.Sample(NearestSampler, uv).r;
        clip(input.position.z > layerDepth ? 1.0f : -1.0f);
    }
#endif
    return float4(input.color, g_Globals.meshOpacity);
}
