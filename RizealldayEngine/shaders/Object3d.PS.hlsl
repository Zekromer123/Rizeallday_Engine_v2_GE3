#include "Object3d.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    int32_t lightingType;  // 0=None, 1=Lambert, 2=HalfLambert
    float32_t2 pad;
};

struct DirectionalLightData
{
    float32_t4 color;
    float32_t3 direction;
    float32_t intensity;
    int32_t active;
    float32_t3 pad;
};

struct PointLightData
{
    float32_t4 color;
    float32_t3 position;
    float32_t intensity;
    float32_t radius;
    float32_t decay;
    int32_t active;
    float32_t pad;
};

struct SpotLightData
{
    float32_t4 color;
    float32_t3 position;
    float32_t intensity;
    float32_t3 direction;
    float32_t distance;
    float32_t decay;
    float32_t cosAngle;
    float32_t cosFalloffStart;
    int32_t active;
};

ConstantBuffer<Material> gMaterial : register(b0);

cbuffer LightGroup : register(b1)
{
    float32_t3 ambientColor;
    float32_t ambientIntensity;
    DirectionalLightData dirLight;
    PointLightData pointLights[3];
    SpotLightData spotLights[3];
};

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    float32_t4 texColor = gTexture.Sample(gSampler, input.texcoord);
    PixelShaderOutput output;
    
    if (gMaterial.enableLighting == 0)
    {
        output.color = gMaterial.color * texColor;
        return output;
    }
    
    float32_t3 N = normalize(input.normal);
    float32_t3 worldPos = input.worldPosition;
    float32_t3 totalLight = ambientColor * ambientIntensity;
    
    // Directional Light
    if (dirLight.active != 0)
    {
        float32_t NdotL = dot(N, -dirLight.direction);
        float32_t diffuse;
        if (gMaterial.lightingType == 2)
        {
            diffuse = pow(NdotL * 0.5f + 0.5f, 2.0f); // Half-Lambert
        }
        else
        {
            diffuse = saturate(NdotL); // Lambert
        }
        totalLight += dirLight.color.rgb * diffuse * dirLight.intensity;
    }
    
    // Point Lights
    for (int32_t i = 0; i < 3; i++)
    {
        if (pointLights[i].active != 0)
        {
            float32_t3 lightVec = worldPos - pointLights[i].position;
            float32_t dist = length(lightVec);
            float32_t3 lightDir = normalize(lightVec);
            float32_t NdotL = saturate(dot(N, -lightDir));
            float32_t attenuation = pow(saturate(1.0f - dist / pointLights[i].radius), pointLights[i].decay);
            totalLight += pointLights[i].color.rgb * NdotL * pointLights[i].intensity * attenuation;
        }
    }
    
    // Spot Lights
    for (int32_t j = 0; j < 3; j++)
    {
        if (spotLights[j].active != 0)
        {
            float32_t3 lightVec = worldPos - spotLights[j].position;
            float32_t dist = length(lightVec);
            float32_t3 lightDir = normalize(lightVec);
            float32_t NdotL = saturate(dot(N, -lightDir));
            float32_t distAtt = pow(saturate(1.0f - dist / spotLights[j].distance), spotLights[j].decay);
            float32_t cosTheta = dot(lightDir, spotLights[j].direction);
            float32_t spotFactor = saturate((cosTheta - spotLights[j].cosFalloffStart) / (spotLights[j].cosAngle - spotLights[j].cosFalloffStart));
            totalLight += spotLights[j].color.rgb * NdotL * spotLights[j].intensity * distAtt * spotFactor;
        }
    }
    
    output.color = float32_t4(gMaterial.color.rgb * texColor.rgb * totalLight, gMaterial.color.a * texColor.a);
    return output;
}
