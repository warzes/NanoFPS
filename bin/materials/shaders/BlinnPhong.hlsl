#include "Config.hlsli"
#include "ppx/BRDF.hlsli"

ConstantBuffer<SceneData>    Scene    : register(SCENE_CONSTANTS_REGISTER,    SCENE_DATA_SPACE);
ConstantBuffer<MaterialData> Material : register(MATERIAL_CONSTANTS_REGISTER, MATERIAL_DATA_SPACE);
ConstantBuffer<ModelData>    Model    : register(MODEL_CONSTANTS_REGISTER,    MODEL_DATA_SPACE);

StructuredBuffer<Light> Lights : register(LIGHT_DATA_REGISTER, SCENE_DATA_SPACE);

Texture2D    AlbedoTex      : register(ALBEDO_TEXTURE_REGISTER,     MATERIAL_RESOURCES_SPACE);
Texture2D    RoughnessTex   : register(ROUGHNESS_TEXTURE_REGISTER,  MATERIAL_RESOURCES_SPACE);
Texture2D    MetalnessTex   : register(METALNESS_TEXTURE_REGISTER,  MATERIAL_RESOURCES_SPACE);
Texture2D    NormalMapTex   : register(NORMAL_MAP_TEXTURE_REGISTER, MATERIAL_RESOURCES_SPACE);
SamplerState ClampedSampler : register(CLAMPED_SAMPLER_REGISTER,    MATERIAL_RESOURCES_SPACE);

float4 psmain(VSOutput input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 V = normalize(Scene.eyePosition.xyz - input.positionWS);

    float3 albedo = Material.albedo;
    if (Material.albedoSelect == 1) {
        albedo = AlbedoTex.Sample(ClampedSampler, input.texCoord).rgb;
        albedo = RemoveGamma(albedo, 2.2);
    }

    float roughness = Material.roughness;
    if (Material.roughnessSelect == 1) {
        roughness = RoughnessTex.Sample(ClampedSampler, input.texCoord).r;
    }

    float metalness = Material.metalness;
    if (Material.metalnessSelect == 1) {
        metalness = MetalnessTex.Sample(ClampedSampler, input.texCoord).r;
    }

    float hardness = lerp(1.0, 100.0, 1.0 - saturate(roughness));

    float3 diffuse  = 0;
    float3 specular = 0;
    for (uint i = 0; i < Scene.lightCount; ++i) {
        float3 L = normalize(Lights[i].position - input.positionWS);

        diffuse += BRDF_Gouraud(N, L);

        specular += BRDF_BlinnPhong(N, V, L, hardness) * Lights[i].intensity;
    }

    float3 finalColor = (diffuse + Scene.ambient) * albedo + specular;

    finalColor = ApplyGamma(finalColor, 2.2);

    return float4(finalColor, 1);
}
