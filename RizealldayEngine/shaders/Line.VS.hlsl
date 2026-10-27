// Debug/editor line renderer (PrimitiveDrawer). View-projection is passed as root constants.
struct LineConstants
{
    float4x4 viewProjection;
};
ConstantBuffer<LineConstants> gLine : register(b0);

struct LineVSInput
{
    float4 position : POSITION0;
    float4 color : COLOR0;
};

struct LineVSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

LineVSOutput main(LineVSInput input)
{
    LineVSOutput output;
    output.position = mul(input.position, gLine.viewProjection);
    output.color = input.color;
    return output;
}
