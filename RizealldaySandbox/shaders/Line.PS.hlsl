struct LineVSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

float4 main(LineVSOutput input) : SV_TARGET0
{
    return input.color;
}
