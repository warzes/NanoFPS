#include "GBuffer.hlsli"

Texture2D    GBufferRT0     : register(GBUFFER_RT0_REGISTER,     GBUFFER_SPACE);
Texture2D    GBufferRT1     : register(GBUFFER_RT1_REGISTER,     GBUFFER_SPACE);
Texture2D    GBufferRT2     : register(GBUFFER_RT2_REGISTER,     GBUFFER_SPACE);
Texture2D    GBufferRT3     : register(GBUFFER_RT3_REGISTER,     GBUFFER_SPACE);
SamplerState ClampedSampler : register(GBUFFER_SAMPLER_REGISTER, GBUFFER_SPACE);

cbuffer GBufferData : register(GBUFFER_CONSTANTS_REGISTER, GBUFFER_SPACE)
{
    uint enableIBL;
    uint enableEnv;
    uint debugAttrIndex;
};

struct VSOutput {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

VSOutput vsmain(uint id : SV_VertexID)
{
    VSOutput result;
    
    // Clip space position
	result.Position.x = (float)(id / 2) * 4.0 - 1.0;    
    result.Position.y = (float)(id % 2) * 4.0 - 1.0;
    result.Position.z = 0.0;
    result.Position.w = 1.0;
    
    // Texture coordinates
    result.TexCoord.x = (float)(id / 2) * 2.0;
    result.TexCoord.y = 1.0 - (float)(id % 2) * 2.0;
    
	return result;
}

float4 psmain(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD) : SV_TARGET
{
    PackedGBuffer packed = (PackedGBuffer) 0;
    packed.rt0 = GBufferRT0.Sample(ClampedSampler, TexCoord);
    packed.rt1 = GBufferRT1.Sample(ClampedSampler, TexCoord);
    packed.rt2 = GBufferRT2.Sample(ClampedSampler, TexCoord);
    packed.rt3 = GBufferRT3.Sample(ClampedSampler, TexCoord);
    
    GBuffer gbuffer = UnpackGBuffer(packed);
    
    float3 color = (float3)0;
    switch (debugAttrIndex) {
        default: break;
        case GBUFFER_POSITION     : color = gbuffer.position; break;
        case GBUFFER_NORMAL       : color = gbuffer.normal; break;
        case GBUFFER_ALBEDO       : color = gbuffer.albedo; break;
        case GBUFFER_F0           : color = gbuffer.F0; break;
        case GBUFFER_ROUGHNESS    : color = gbuffer.roughness; break;
        case GBUFFER_METALNESS    : color = gbuffer.metalness; break;
        case GBUFFER_AMB_OCC      : color = gbuffer.ambOcc; break;
        case GBUFFER_IBL_STRENGTH : color = gbuffer.iblStrength; break;
        case GBUFFER_ENV_STRENGTH : color = gbuffer.envStrength; break;
    }

    return float4(color, 1.0);
}