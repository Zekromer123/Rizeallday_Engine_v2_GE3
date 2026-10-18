struct SkyboxVSOutput
{
    float4 position : SV_POSITION;
    float3 viewDir : TEXCOORD0;
};

struct SkyboxConstants
{
    float4x4 invViewProj;
    float4 topColor;
    float4 horizonColor;
    float4 bottomColor;
    float4 solidColor;
    float rotation;
    float exposure;
    int mode; // 0: Gradient, 1: SolidColor, 2: Cubemap
    float pad;
};

ConstantBuffer<SkyboxConstants> gSkybox : register(b0);
TextureCube<float4> gSkyCubemap : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(SkyboxVSOutput input)
{
    PixelShaderOutput output;
    
    // Mode 1: Solid Color
    if (gSkybox.mode == 1)
    {
        output.color = gSkybox.solidColor;
        return output;
    }
    
    float3 dir = normalize(input.viewDir);
    
    // Mode 2: Cubemap
    if (gSkybox.mode == 2)
    {
        float rad = radians(gSkybox.rotation);
        float cosR = cos(rad);
        float sinR = sin(rad);
        float3 rotDir = float3(
            dir.x * cosR + dir.z * sinR,
            dir.y,
            -dir.x * sinR + dir.z * cosR
        );
        
        float4 skySample = gSkyCubemap.Sample(gSampler, rotDir);
        output.color = float4(skySample.rgb * gSkybox.exposure, 1.0f);
        return output;
    }
    
    // Mode 0: Gradient
    // dir.y is elevation angle (-1 bottom to +1 top)
    float t = saturate(dir.y);
    float4 col = lerp(gSkybox.horizonColor, gSkybox.topColor, t);
    if (dir.y < 0.0f)
    {
        col = lerp(gSkybox.horizonColor, gSkybox.bottomColor, saturate(-dir.y * 2.0f));
    }
    
    output.color = col;
    output.color.a = 1.0f;
    return output;
}
