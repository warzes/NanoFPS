struct SceneData
{
    float4x4 ModelMatrix;                // Transforms object space to world space.
    float4x4 ITModelMatrix;              // Inverse transpose of the ModelMatrix.
    float4   Ambient;                    // Object's ambient intensity.
    float4x4 CameraViewProjectionMatrix; // Camera's view projection matrix.
    float4   LightPosition;              // Light's position.
    float4   EyePosition;                // Eye (camera) position.
};

ConstantBuffer<SceneData> Scene : register(b0);
Texture2D                 AlbedoTexture         : register(t1);
SamplerState              AlbedoSampler         : register(s2);

struct VSOutput {
  float4 world_position : POSITION;
  float4 position       : SV_POSITION;
  float2 uv             : TEXCOORD;
  float3 normal         : NORMAL;
  float3 normalTS    : NORMALTS;
  float3 tangentTS   : TANGENTTS;
  float3 bitangentTS : BITANGENTTS;
};

float4 psmain(VSOutput input) : SV_TARGET
{
    const float4 albedo = AlbedoTexture.Sample(AlbedoSampler, input.uv).rgba;
    if (albedo.a < 0.8f) {
      discard;
    }
    return float4(albedo.rgb, 1.f);
}
