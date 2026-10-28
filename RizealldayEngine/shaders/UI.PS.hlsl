Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct UIVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

float4 main(UIVSOutput input) : SV_TARGET0
{
    return gTexture.Sample(gSampler, input.texcoord) * input.color;
}
