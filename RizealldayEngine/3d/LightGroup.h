#pragma once
#include "3d/DirectionalLight.h"
#include "3d/PointLight.h"
#include "3d/SpotLight.h"
#include "math/Vector3.h"
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

class DirectXCommon;

struct LightGroupGPU {
    Vector3 ambientColor;
    float ambientIntensity;
    DirectionalLightData directionalLight;
    PointLightData pointLights[3];
    SpotLightData spotLights[3];
};

class LightGroup {
public:
    static const int32_t MaxPointLights = 3;
    static const int32_t MaxSpotLights = 3;

    static void StaticInitialize(DirectXCommon* dxCommon);
    static LightGroup* Create();

    void Initialize();
    void Update();
    void Draw(ID3D12GraphicsCommandList* cmdList, uint32_t rootParameterIndex);
    void SetDefaultLighting();

    void SetAmbientColor(const Vector3& color) { ambientColor_ = color; }
    void SetAmbientIntensity(float intensity) { ambientIntensity_ = intensity; }
    
    DirectionalLightData& GetDirectionalLight() { return directionalLight_; }
    PointLightData& GetPointLight(int32_t index) { return pointLights_[index]; }
    SpotLightData& GetSpotLight(int32_t index) { return spotLights_[index]; }

private:
    static DirectXCommon* sDxCommon_;

    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
    LightGroupGPU* mappedData_ = nullptr;

    Vector3 ambientColor_ = {1.0f, 1.0f, 1.0f};
    float ambientIntensity_ = 0.1f;
    
    DirectionalLightData directionalLight_;
    PointLightData pointLights_[MaxPointLights];
    SpotLightData spotLights_[MaxSpotLights];
};
