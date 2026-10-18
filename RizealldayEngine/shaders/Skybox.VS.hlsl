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

SkyboxVSOutput main(uint vertexId : SV_VertexID)
{
    SkyboxVSOutput output;
    
    // Fullscreen triangle covering clip space [-1, 1]
    // vertexId 0 -> (-1, 1), 1 -> (3, 1), 2 -> (-1, -3)
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    float4 pos = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, 1.0f, 1.0f);
    output.position = pos;
    
    // Unproject far-plane position into world-space view direction
    float4 worldPos = mul(pos, gSkybox.invViewProj);
    output.viewDir = worldPos.xyz / worldPos.w;
    
    return output;
}
