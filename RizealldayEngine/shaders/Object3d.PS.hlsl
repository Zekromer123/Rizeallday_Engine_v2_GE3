#include "Object3d.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    int32_t lightingType;  // 0=Unlit, 1=Lambert, 2=HalfLambert, 3=Phong, 4=BlinnPhong
    float32_t shininess;
    int32_t receiveShadows;
    float32_t3 specularColor;
    float32_t explodeAmount;
    float32_t2 uvOffset;
    float32_t2 uvTiling;
    float32_t uvRotation;
    float32_t3 pad;
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

ConstantBuffer<Material> gMaterial : register(b3);

cbuffer LightGroup : register(b1)
{
    float32_t3 ambientColor;
    float32_t ambientIntensity;
    DirectionalLightData dirLight;
    PointLightData pointLights[3];
    SpotLightData spotLights[3];
    float32_t4x4 lightViewProjection;
    int32_t shadowActive;
    float32_t shadowBias;
    float32_t shadowStrength;
    float32_t shadowNormalBias;
};

struct CameraForGPU
{
    float32_t3 worldPosition;
    float32_t pad;
};

ConstantBuffer<CameraForGPU> gCamera : register(b2);

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

Texture2D<float> gShadowMap : register(t1);
SamplerComparisonState gShadowSampler : register(s1);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

float32_t CalculateShadow(float32_t3 worldPos, float32_t3 normal, float32_t3 lightDir)
{
    if (shadowActive == 0)
    {
        return 1.0f;
    }

    // Normal offset bias to prevent shadow acne on curved/sloped surfaces
    float32_t NdotL = dot(normal, -lightDir);
    float32_t normalBias = shadowNormalBias * saturate(1.0f - saturate(NdotL));
    float32_t3 biasedWorldPos = worldPos + normal * normalBias;

    float32_t4 lightSpacePos = mul(float32_t4(biasedWorldPos, 1.0f), lightViewProjection);
    if (lightSpacePos.w <= 0.0001f)
    {
        return 1.0f;
    }

    float32_t3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    // Convert NDC [-1, 1] to UV [0, 1] (invert Y for texture coords)
    float32_t2 shadowUV;
    shadowUV.x = projCoords.x * 0.5f + 0.5f;
    shadowUV.y = -projCoords.y * 0.5f + 0.5f;

    // Outside the light frustum: not in shadow
    if (shadowUV.x < 0.0f || shadowUV.x > 1.0f ||
        shadowUV.y < 0.0f || shadowUV.y > 1.0f ||
        projCoords.z < 0.0f || projCoords.z > 1.0f)
    {
        return 1.0f;
    }

    // Slope-scaled depth bias
    float32_t cosTheta = saturate(NdotL);
    float32_t slope = sqrt(saturate(1.0f - cosTheta * cosTheta)) / max(0.001f, cosTheta);
    float32_t slopeBias = shadowBias * min(slope, 3.0f);
    float32_t compareDepth = projCoords.z - (shadowBias + slopeBias);

    // 3x3 PCF (Percentage Closer Filtering)
    float32_t shadow = 0.0f;
    const float32_t texelSize = 1.0f / 2048.0f;
    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            float32_t2 offset = float32_t2(x, y) * texelSize;
            shadow += gShadowMap.SampleCmpLevelZero(gShadowSampler, shadowUV + offset, compareDepth);
        }
    }
    return shadow / 9.0f;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    // UV transformation: Tiling, Rotation, Offset
    float32_t2 uv = input.texcoord;
    uv = uv * gMaterial.uvTiling;
    float cosRot = cos(gMaterial.uvRotation);
    float sinRot = sin(gMaterial.uvRotation);
    uv = float32_t2(
        uv.x * cosRot - uv.y * sinRot,
        uv.x * sinRot + uv.y * cosRot
    );
    uv = uv + gMaterial.uvOffset;

    float32_t4 texColor = gTexture.Sample(gSampler, uv);
    PixelShaderOutput output;
    
    if (gMaterial.enableLighting == 0 || gMaterial.lightingType == 0)
    {
        output.color = gMaterial.color * texColor;
        return output;
    }
    
    float32_t3 N = normalize(input.normal);
    float32_t3 worldPos = input.worldPosition;
    float32_t3 toEye = normalize(gCamera.worldPosition - worldPos);
    
    float32_t3 totalDiffuse = ambientColor * ambientIntensity;
    float32_t3 totalSpecular = float32_t3(0.0f, 0.0f, 0.0f);
    
    float shininess = max(1.0f, gMaterial.shininess);
    float32_t3 specColor = gMaterial.specularColor;
    
    // 1. Directional Light
    if (dirLight.active != 0)
    {
        float32_t3 lightDir = normalize(dirLight.direction);
        float32_t NdotL = dot(N, -lightDir);
        float32_t diffuse = 0.0f;
        
        if (gMaterial.lightingType == 2) {
            diffuse = pow(NdotL * 0.5f + 0.5f, 2.0f); // Half-Lambert
        } else {
            diffuse = saturate(NdotL); // Lambert / Phong / Blinn-Phong
        }
        
        float32_t specularPow = 0.0f;
        if (gMaterial.lightingType == 3) {
            // Phong (02_00): reflect(lightDir, normal)
            float32_t3 reflectLight = reflect(lightDir, N);
            specularPow = pow(saturate(dot(reflectLight, toEye)), shininess);
        } else if (gMaterial.lightingType == 4) {
            // Blinn-Phong (02_01): normalize(-lightDir + toEye)
            float32_t3 halfVector = normalize(-lightDir + toEye);
            specularPow = pow(saturate(dot(N, halfVector)), shininess);
        }

        // Shadow calculation: modulates Directional Light diffuse and specular only
        float32_t shadowFactor = 1.0f;
        if (gMaterial.receiveShadows != 0)
        {
            float32_t shadow = CalculateShadow(worldPos, N, lightDir);
            shadowFactor = 1.0f - (1.0f - shadow) * shadowStrength;
        }
        
        totalDiffuse += dirLight.color.rgb * diffuse * dirLight.intensity * shadowFactor;
        totalSpecular += dirLight.color.rgb * specColor * specularPow * dirLight.intensity * shadowFactor;
    }
    
    // 2. Point Lights (03_00)
    for (int32_t i = 0; i < 3; i++)
    {
        if (pointLights[i].active != 0)
        {
            float32_t3 lightVec = worldPos - pointLights[i].position;
            float32_t dist = length(lightVec);
            float32_t3 lightDir = normalize(lightVec);
            
            // Attenuation: pow(saturate(-distance / radius + 1.0f), decay)
            float32_t attenuation = pow(saturate(-dist / max(0.001f, pointLights[i].radius) + 1.0f), max(0.001f, pointLights[i].decay));
            
            float32_t NdotL = dot(N, -lightDir);
            float32_t diffuse = 0.0f;
            if (gMaterial.lightingType == 2) {
                diffuse = pow(NdotL * 0.5f + 0.5f, 2.0f);
            } else {
                diffuse = saturate(NdotL);
            }
            
            float32_t specularPow = 0.0f;
            if (gMaterial.lightingType == 3) {
                float32_t3 reflectLight = reflect(lightDir, N);
                specularPow = pow(saturate(dot(reflectLight, toEye)), shininess);
            } else if (gMaterial.lightingType == 4) {
                float32_t3 halfVector = normalize(-lightDir + toEye);
                specularPow = pow(saturate(dot(N, halfVector)), shininess);
            }
            
            totalDiffuse += pointLights[i].color.rgb * diffuse * pointLights[i].intensity * attenuation;
            totalSpecular += pointLights[i].color.rgb * specColor * specularPow * pointLights[i].intensity * attenuation;
        }
    }
    
    // 3. Spot Lights (03_01)
    for (int32_t j = 0; j < 3; j++)
    {
        if (spotLights[j].active != 0)
        {
            float32_t3 lightVec = worldPos - spotLights[j].position;
            float32_t dist = length(lightVec);
            float32_t3 lightDir = normalize(lightVec);
            
            float32_t cosTheta = dot(lightDir, normalize(spotLights[j].direction));
            float32_t denom = max(0.001f, spotLights[j].cosFalloffStart - spotLights[j].cosAngle);
            float32_t falloffFactor = saturate((cosTheta - spotLights[j].cosAngle) / denom);
            float32_t attenuation = pow(saturate(-dist / max(0.001f, spotLights[j].distance) + 1.0f), max(0.001f, spotLights[j].decay)) * falloffFactor;
            
            float32_t NdotL = dot(N, -lightDir);
            float32_t diffuse = 0.0f;
            if (gMaterial.lightingType == 2) {
                diffuse = pow(NdotL * 0.5f + 0.5f, 2.0f);
            } else {
                diffuse = saturate(NdotL);
            }
            
            float32_t specularPow = 0.0f;
            if (gMaterial.lightingType == 3) {
                float32_t3 reflectLight = reflect(lightDir, N);
                specularPow = pow(saturate(dot(reflectLight, toEye)), shininess);
            } else if (gMaterial.lightingType == 4) {
                float32_t3 halfVector = normalize(-lightDir + toEye);
                specularPow = pow(saturate(dot(N, halfVector)), shininess);
            }
            
            totalDiffuse += spotLights[j].color.rgb * diffuse * spotLights[j].intensity * attenuation;
            totalSpecular += spotLights[j].color.rgb * specColor * specularPow * spotLights[j].intensity * attenuation;
        }
    }
    
    float32_t3 finalColor = gMaterial.color.rgb * texColor.rgb * totalDiffuse + totalSpecular;
    output.color = float32_t4(finalColor, gMaterial.color.a * texColor.a);
    return output;
}
