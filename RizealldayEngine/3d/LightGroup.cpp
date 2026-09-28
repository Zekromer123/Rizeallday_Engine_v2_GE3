#include "3d/LightGroup.h"
#include "base/DirectXCommon.h"
#include <cassert>

DirectXCommon* LightGroup::sDxCommon_ = nullptr;

void LightGroup::StaticInitialize(DirectXCommon* dxCommon) {
    sDxCommon_ = dxCommon;
}

LightGroup* LightGroup::Create() {
    LightGroup* instance = new LightGroup();
    instance->Initialize();
    return instance;
}

void LightGroup::Initialize() {
    assert(sDxCommon_);
    
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = (sizeof(LightGroupGPU) + 0xff) & ~0xff; // 256-byte alignment
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    
    HRESULT hr = sDxCommon_->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&constBuffer_)
    );
    assert(SUCCEEDED(hr));
    
    hr = constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData_));
    assert(SUCCEEDED(hr));

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
    if (mappedData_) {
        mappedData_->ambientColor = ambientColor_;
        mappedData_->ambientIntensity = ambientIntensity_;
        mappedData_->directionalLight = directionalLight_;
        
        for (int i = 0; i < MaxPointLights; ++i) {
            mappedData_->pointLights[i] = pointLights_[i];
        }
        
        for (int i = 0; i < MaxSpotLights; ++i) {
            mappedData_->spotLights[i] = spotLights_[i];
        }
    }
}

void LightGroup::Draw(ID3D12GraphicsCommandList* cmdList, uint32_t rootParameterIndex) {
    if (constBuffer_) {
        cmdList->SetGraphicsRootConstantBufferView(rootParameterIndex, constBuffer_->GetGPUVirtualAddress());
    }
}
