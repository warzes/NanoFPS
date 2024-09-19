struct DrawParams
{
    float4x4 MVP;
};

ConstantBuffer<DrawParams> Draw     : register(b0);
Texture2D                  Texture0 : register(t1);
SamplerState               Sampler0 : register(s4);

struct VSOutput {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

VSOutput vsmain(float4 Position : POSITION, float2 TexCoord : TEXCOORD0)
{
    VSOutput result;
    result.Position = mul(Draw.MVP, Position);
    result.TexCoord = TexCoord;
    return result;
}

float4 psmain(VSOutput input) : SV_TARGET
{
    return Texture0.Sample(Sampler0, input.TexCoord);
}
