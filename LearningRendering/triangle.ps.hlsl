struct PixelInput
{
    float3 color : COLOR;
};

float4 main_PS(PixelInput pixelInput) : SV_Target0
{
    float3 inColor = pixelInput.color;
    return float4(inColor, 1.0f);
}
