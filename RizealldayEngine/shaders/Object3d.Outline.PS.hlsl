#include "Object3d.hlsli"

// Same layout as Material in math/VertexData.h
struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    int32_t lightingType;
    float32_t shininess;
    int32_t receiveShadows;
    float32_t3 specularColor;
    float32_t explodeAmount;
    float32_t2 uvOffset;
    float32_t2 uvTiling;
    float32_t uvRotation;
    float32_t outlineWidth;
    float32_t2 pad;
    float32_t4 outlineColor;
};
ConstantBuffer<Material> gMaterial : register(b3);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gMaterial.outlineColor;
    return output;
}
