#include "Object3d.hlsli"

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
    float32_t3 pad;
};

ConstantBuffer<Material> gMaterial : register(b3);

[maxvertexcount(3)]
void main(triangle VertexShaderOutput input[3], inout TriangleStream<VertexShaderOutput> output)
{
    // Compute clip-space face normal to separate faces visually in screen/depth space
    float3 p0 = input[0].position.xyz / input[0].position.w;
    float3 p1 = input[1].position.xyz / input[1].position.w;
    float3 p2 = input[2].position.xyz / input[2].position.w;

    float3 edge0 = p1 - p0;
    float3 edge1 = p2 - p0;
    float3 clipNormal = cross(edge0, edge1);
    float len = length(clipNormal);
    if (len > 0.0001f) {
        clipNormal /= len;
    } else {
        clipNormal = float3(0.0f, 0.0f, 1.0f);
    }

    // Compute world-space face normal for lighting calculation
    float3 wEdge0 = input[1].worldPosition - input[0].worldPosition;
    float3 wEdge1 = input[2].worldPosition - input[0].worldPosition;
    float3 worldFaceNormal = cross(wEdge0, wEdge1);
    float wLen = length(worldFaceNormal);
    if (wLen > 0.0001f) {
        worldFaceNormal /= wLen;
    } else {
        worldFaceNormal = normalize(input[0].normal + input[1].normal + input[2].normal);
    }

    float amount = gMaterial.explodeAmount;

    for (int i = 0; i < 3; ++i)
    {
        VertexShaderOutput v = input[i];
        v.worldPosition += worldFaceNormal * amount;
        v.position.xyz += clipNormal * amount * v.position.w;
        output.Append(v);
    }
    output.RestartStrip();
}
