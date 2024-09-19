#define IS_SHADER
#include "Common.hlsli"
#include "FullscreenVS.hlsli"

SamplerState NearestSampler                            : register(CUSTOM_SAMPLER_0_REGISTER);
Texture2D    LayerTextures[DEPTH_PEELING_LAYERS_COUNT] : register(CUSTOM_TEXTURE_0_REGISTER);

float4 psmain(VSOutput input) : SV_TARGET
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    for(int i = g_Globals.depthPeelingBackLayerIndex; i >= g_Globals.depthPeelingFrontLayerIndex; --i)
    {
        MergeColor(color, LayerTextures[i].Sample(NearestSampler, input.uv));
    }
    color.a = 1.0f - color.a;
    return color;
}
