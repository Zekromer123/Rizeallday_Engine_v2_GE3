#include "3d/LightGroup.h"
#include "base/DirectXCommon.h"
#include <cassert>

DirectXCommon* LightGroup::sDxCommon_ = nullptr;

void LightGroup::StaticInitialize(DirectXCommon* dxCommon) {
    sDxCommon_ = dxCommon;
}

std::unique_ptr<LightGroup> LightGroup::Create() {
    auto instance = std::make_unique<LightGroup>(PrivateTag{});
    instance->Initialize();
    return instance;
}

void LightGroup::Initialize() {
    assert(sDxCommon_);
    
    SetDefaultLighting();
    Update();
}

void LightGroup::SetDefaultLighting() {
    ambientColor_ = {1.0f, 1.0f, 1.0f};
    ambientIntensity_ = 0.1f;
    
    directionalLight_.active = 1;
    directionalLight_.color = {1.0f, 1.0f, 1.0f, 1.0f};
    directionalLight_.direction = {0.0f, -1.0f, 0.0f};
    directionalLight_.intensity = 1.0f;
    
    for (int i = 0; i < MaxPointLights; ++i) {
        pointLights_[i].active = 0;
    }
    
    for (int i = 0; i < MaxSpotLights; ++i) {
        spotLights_[i].active = 0;
    }
}

void LightGroup::Update() {
    data_.ambientColor = ambientColor_;
    data_.ambientIntensity = ambientIntensity_;
    data_.directionalLight = directionalLight_;
    
    for (int i = 0; i < MaxPointLights; ++i) {
        data_.pointLights[i] = pointLights_[i];
    }
    
    for (int i = 0; i < MaxSpotLights; ++i) {
        data_.spotLights[i] = spotLights_[i];
    }

    data_.lightViewProjection = lightViewProjection_;
    data_.shadowActive = shadowActive_;
    data_.shadowBias = shadowBias_;
    data_.shadowStrength = shadowStrength_;
    data_.shadowNormalBias = shadowNormalBias_;
    ++version_;
}

void LightGroup::Draw(ID3D12GraphicsCommandList* cmdList, uint32_t rootParameterIndex) {
    if (!sDxCommon_ || sDxCommon_->IsDeviceLost()) {
        return;
    }
    const uint64_t frame = sDxCommon_->GetFrameIndex();
    if (uploadFrame_ != frame || uploadedVersion_ != version_ || uploadedAddress_ == 0) {
        uploadedAddress_ = sDxCommon_->UploadConstants(data_);
        uploadFrame_ = frame;
        uploadedVersion_ = version_;
    }
    if (uploadedAddress_ != 0) {
        cmdList->SetGraphicsRootConstantBufferView(rootParameterIndex, uploadedAddress_);
    }
}
