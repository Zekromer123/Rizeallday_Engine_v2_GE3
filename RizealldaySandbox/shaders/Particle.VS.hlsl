#include "Object3d.hlsli"

struct TransformationMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 World;
    float32_t4 color;
};

// Root Parameter 1: Descriptor Table -> StructuredBuffer (t0, Vertex Shader)
StructuredBuffer<TransformationMatrix> gTransformationMatrices : register(t0);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
    uint4 boneIndices : BLENDINDICES0;
    float4 boneWeights : BLENDWEIGHT0;
};

struct ParticleVertexOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
    float32_t3 worldPosition : TEXCOORD1;
    float32_t4 color : COLOR0;
};

ParticleVertexOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    ParticleVertexOutput output;
    
    // Fetch WVP and World matrix from StructuredBuffer by instance ID
    output.position = mul(input.position, gTransformationMatrices[instanceId].WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float32_t3x3)gTransformationMatrices[instanceId].World));
    output.worldPosition = mul(input.position, gTransformationMatrices[instanceId].World).xyz;
    
    // Fetch individual particle color from StructuredBuffer
    output.color = gTransformationMatrices[instanceId].color;
    
    return output;
}
