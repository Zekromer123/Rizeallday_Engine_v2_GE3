#include "Object3d.hlsli"

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

    output.position = mul(skinnedPos, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(skinnedNormal, (float32_t3x3) gTransformationMatrix.WorldInverseTranspose));
    output.worldPosition = mul(skinnedPos, gTransformationMatrix.World).xyz;
    return output;
}