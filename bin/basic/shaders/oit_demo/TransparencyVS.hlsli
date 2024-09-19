struct VSOutput
{
    float4 position : SV_POSITION;
    float3 color    : COLOR;
};

VSOutput vsmain(float4 position : POSITION)
{
    VSOutput result;
    result.position = mul(g_Globals.meshMVP, position);
    result.color    = abs(position.xyz);
    return result;
}
