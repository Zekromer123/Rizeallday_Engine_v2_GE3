#include "3d/Object3d.h"
#include "base/DirectXCommon.h"
#include "3d/Model.h"

DirectXCommon* Object3d::sDxCommon_ = nullptr;

void Object3d::StaticInitialize(DirectXCommon* dxCommon) {
    sDxCommon_ = dxCommon;
}

void Object3d::Initialize() {
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = sizeof(Material);
    desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1;
    desc.SampleDesc.Count = 1; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    sDxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&materialResource_));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    if (materialData_) {
        materialData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
        materialData_->enableLighting = 1;
        materialData_->lightingType = 2;
    }

    worldTransform.Initialize();
}

void Object3d::Update() {
    if (camera_) {
        worldTransform.UpdateMatrix(camera_->GetViewProjectionMatrix());
    } else {
        worldTransform.UpdateMatrix(Mathematics::MakeIdentity4x4());
    }
}

void Object3d::Draw(ID3D12GraphicsCommandList* cmdList) {
    cmdList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, worldTransform.GetGPUAddress());
    if (model_) {
        model_->Draw(cmdList);
    }
}
