#include "Config.hlsli"

ConstantBuffer<SceneData>    Scene    : register(SCENE_CONSTANTS_REGISTER,    SCENE_DATA_SPACE);
StructuredBuffer<Light>      Lights   : register(LIGHT_DATA_REGISTER,         SCENE_DATA_SPACE);
ConstantBuffer<MaterialData> Material : register(MATERIAL_CONSTANTS_REGISTER, MATERIAL_DATA_SPACE);
ConstantBuffer<ModelData>    Model    : register(MODEL_CONSTANTS_REGISTER,    MODEL_DATA_SPACE);

VSOutput vsmain(VSInput input)
{
    VSOutput output = (VSOutput)0;
    
    output.positionWS = mul(Model.modelMatrix, float4(input.position, 1)).xyz;
    output.position   = mul(Scene.viewProjectionMatrix, float4(output.positionWS, 1));
    output.normal     = mul(Model.normalMatrix, float4(input.normal, 0)).xyz;

    output.color    = input.color;
    output.texCoord = input.texCoord;
    
    // TBN
    output.normalTS    = mul(Model.modelMatrix, float4(input.normal, 0)).xyz;
    output.tangentTS   = mul(Model.modelMatrix, float4(input.tangent, 0)).xyz;
    output.bitangnetTS = mul(Model.modelMatrix, float4(input.bitangnet, 0)).xyz;    
    
    return output;
}