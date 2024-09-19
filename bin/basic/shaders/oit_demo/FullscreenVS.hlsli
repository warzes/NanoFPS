struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

VSOutput vsmain(uint VertexID : SV_VertexID)
{
    const float4 vertices[] =
    {
        float4(-1.0f, -1.0f, +0.0f, +1.0f),
        float4(+3.0f, -1.0f, +2.0f, +1.0f),
        float4(-1.0f, +3.0f, +0.0f, -1.0f),
    };

    VSOutput result;
    result.position = float4(vertices[VertexID].xy, 0.0f, 1.0f);
    result.uv       = float2(vertices[VertexID].zw);
    return result;
}
