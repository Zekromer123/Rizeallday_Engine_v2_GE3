#include "Object3d.hlsli"

// Inverted-hull outline: the mesh pushed out along its normals by outlineWidth (world units), drawn with
// front faces culled after the normal pass. Skinning must match Object3d.VS.hlsl.

struct TransformationMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 World;
    float32_t4x4 WorldInverseTranspose;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

cbuffer BonePalette : register(b1)
{
    float32_t4x4 gBones[128];
};

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

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
    uint4 boneIndices : BLENDINDICES0;
    float4 boneWeights : BLENDWEIGHT0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    float32_t4 skinnedPos = input.position;
    float32_t3 skinnedNormal = input.normal;
    float32_t totalWeight = input.boneWeights.x + input.boneWeights.y + input.boneWeights.z + input.boneWeights.w;
    if (totalWeight > 0.0001f)
    {
        float32_t4x4 skinMatrix =
            gBones[input.boneIndices.x] * input.boneWeights.x +
            gBones[input.boneIndices.y] * input.boneWeights.y +
            gBones[input.boneIndices.z] * input.boneWeights.z +
            gBones[input.boneIndices.w] * input.boneWeights.w;
        skinnedPos = mul(float32_t4(input.position.xyz, 1.0f), skinMatrix);
        skinnedNormal = mul(input.normal, (float32_t3x3)skinMatrix);
    }

    // World-space push, taken back to object space (inverse(World) = transpose(WorldInverseTranspose)),
    // so the outline keeps its width on scaled models (e.g. 0.01 Mixamo characters)
    float32_t3x3 wit = (float32_t3x3)gTransformationMatrix.WorldInverseTranspose;
    float32_t3 worldNormal = normalize(mul(skinnedNormal, wit));
    float32_t3 objectOffset = mul(wit, worldNormal * gMaterial.outlineWidth);
    float32_t4 pushed = float32_t4(skinnedPos.xyz + objectOffset, 1.0f);

    output.position = mul(pushed, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = worldNormal;
    output.worldPosition = mul(pushed, gTransformationMatrix.World).xyz;
    return output;
}
