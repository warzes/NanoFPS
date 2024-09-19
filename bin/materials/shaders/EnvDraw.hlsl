struct TransformData
{
    float4x4 M;
};

ConstantBuffer<TransformData> Transform : register(b0);
Texture2D                     Tex0      : register(t1);
SamplerState                  Sampler0  : register(s2);

struct VSOutput {
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};

VSOutput vsmain(float4 Position : POSITION, float2 TexCoord : TEXCOORD0)
{
	VSOutput result;
	result.Position = mul(Transform.M, Position);
	result.TexCoord = TexCoord;
	return result;
}

//
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/ (MIT licensed)
//
float3 ACESFilm(float3 x){
    return saturate((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14));
}

float4 psmain(VSOutput input) : SV_TARGET
{   
    float3 color = Tex0.Sample(Sampler0, input.TexCoord, 0).rgb;
    color = ACESFilm(color);
    color = pow(color, 1 / 2.2);
    return float4(color, 1);    
}
