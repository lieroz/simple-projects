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

struct PixelInput
{
    float3 color : COLOR;
};

struct PixelOutput
{
    float4 attachment0 : SV_Target0;
};

VertexOutput vs_main(VertexInput vertexInput)
{
    float3 inColor = vertexInput.inColor;
    float3 inPos = vertexInput.inPos;

    VertexOutput output;
    output.position = float4(inPos, 1.f);
    output.color = inColor;
    return output;
}

PixelOutput ps_main(PixelInput pixelInput)
{
    float3 inColor = pixelInput.color;
    PixelOutput output;
    output.attachment0 = float4(inColor, 1.0f);
    return output;
}