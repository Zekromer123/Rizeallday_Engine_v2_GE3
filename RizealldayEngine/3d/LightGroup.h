#pragma once
#include "3d/DirectionalLight.h"
#include "3d/PointLight.h"
#include "3d/SpotLight.h"
#include "math/Vector3.h"
#include "math/Matrix4x4.h"
#include "math/MathUtil.h"
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <memory>

class DirectXCommon;

struct LightGroupGPU {
    Vector3 ambientColor;
    float ambientIntensity;
    DirectionalLightData directionalLight;
    PointLightData pointLights[3];
    SpotLightData spotLights[3];
    Matrix4x4 lightViewProjection;
    int32_t shadowActive;
    float shadowBias;
    float shadowStrength;
    float shadowNormalBias;
};

class LightGroup {
private:
    struct PrivateTag {};

public:
    static const int32_t MaxPointLights = 3;
    static const int32_t MaxSpotLights = 3;

    static void StaticInitialize(DirectXCommon* dxCommon);
    static std::unique_ptr<LightGroup> Create();

    explicit LightGroup(PrivateTag) {}
    ~LightGroup();

    void Initialize();
    void Update();
    void Draw(ID3D12GraphicsCommandList* cmdList, uint32_t rootParameterIndex);
    void SetDefaultLighting();

    void SetAmbientColor(const Vector3& color) { ambientColor_ = color; }
    void SetAmbientIntensity(float intensity) { ambientIntensity_ = intensity; }
    void SetCameraPosition(const Vector3&) {}
    Vector3 GetCameraPosition() const { return Vector3{0.0f, 0.0f, 0.0f}; }

    void SetLightViewProjection(const Matrix4x4& mat) { lightViewProjection_ = mat; }
    void SetShadowActive(int32_t active) { shadowActive_ = active; }
    void SetShadowBias(float bias) { shadowBias_ = bias; }
    void SetShadowStrength(float strength) { shadowStrength_ = strength; }
    void SetShadowNormalBias(float normalBias) { shadowNormalBias_ = normalBias; }
    
    DirectionalLightData& GetDirectionalLight() { return directionalLight_; }
    PointLightData& GetPointLight(int32_t index) { return pointLights_[index]; }
    SpotLightData& GetSpotLight(int32_t index) { return spotLights_[index]; }

private:
    LightGroup() = default;
    static DirectXCommon* sDxCommon_;

    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
    LightGroupGPU* mappedData_ = nullptr;

    Vector3 ambientColor_ = {1.0f, 1.0f, 1.0f};
    float ambientIntensity_ = 0.1f;
    
    DirectionalLightData directionalLight_;
    PointLightData pointLights_[MaxPointLights];
    SpotLightData spotLights_[MaxSpotLights];

    Matrix4x4 lightViewProjection_ = Mathematics::MakeIdentity4x4();
    int32_t shadowActive_ = 1;
    float shadowBias_ = 0.002f;
    float shadowStrength_ = 1.0f;
    float shadowNormalBias_ = 0.02f;
};
