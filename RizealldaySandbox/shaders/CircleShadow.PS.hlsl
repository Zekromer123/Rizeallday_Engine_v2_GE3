// CircleShadow.PS.hlsl
// Pixel shader untuk circle shadow (blob shadow)
// Menggambar lingkaran gelap semi-transparan dengan gradient

#include "Object3d.hlsli"

struct CircleShadowMaterial
{
    float32_t4 color;        // Warna bayangan (biasanya hitam gelap)
    float32_t2 center;       // Pusat bayangan di UV space (0.5, 0.5)
    float32_t  radius;       // Radius lingkaran (0.0 - 1.0 dalam UV space)
    float32_t  softness;     // Kelembutan tepi (0=tajam, 1=sangat halus)
};

ConstantBuffer<CircleShadowMaterial> gMaterial : register(b0);

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // Hitung jarak dari pusat bayangan menggunakan UV
    float32_t2 uv = input.texcoord;
    float32_t2 centeredUV = uv - gMaterial.center;

    // Elips: perlebar di arah X untuk bayangan isometric
    centeredUV.x *= 0.6f; // Skala X agar terlihat seperti elips perspektif

    float32_t dist = length(centeredUV);

    // Hitung alpha dengan smooth falloff dari pusat ke tepi
    float32_t alpha = 1.0f - smoothstep(gMaterial.radius * (1.0f - gMaterial.softness),
                                         gMaterial.radius,
                                         dist);

    // Clamp agar tidak melebihi batas radius
    alpha = saturate(alpha);

    output.color = float32_t4(gMaterial.color.rgb, gMaterial.color.a * alpha);
    return output;
}
