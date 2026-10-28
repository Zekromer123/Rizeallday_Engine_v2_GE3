// Game UI (UIRenderer): vertices arrive in NDC, so the vertex shader only passes them through
struct UIVSInput
{
    float2 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct UIVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

UIVSOutput main(UIVSInput input)
{
    UIVSOutput output;
    output.position = float4(input.position, 0.0f, 1.0f);
    output.texcoord = input.texcoord;
    output.color = input.color;
    return output;
}
