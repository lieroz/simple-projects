struct VertexInput
{
    float3 inPos : POSITION;
    float3 inColor : COLOR;
};

struct VertexOutput
{
    float3 color : COLOR;
    float4 position : SV_Position;
};

VertexOutput main_VS(VertexInput vertexInput)
{
    float3 inColor = vertexInput.inColor;
    float3 inPos = vertexInput.inPos;

    VertexOutput output;
    output.position = float4(inPos, 1.f);
    output.color = inColor;
    return output;
}