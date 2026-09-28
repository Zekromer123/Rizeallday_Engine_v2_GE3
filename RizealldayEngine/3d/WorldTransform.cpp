#include "3d/WorldTransform.h"
#include "base/DirectXCommon.h"

DirectXCommon* WorldTransform::sDxCommon_ = nullptr;

void WorldTransform::StaticInitialize(DirectXCommon* dxCommon) {
    sDxCommon_ = dxCommon;
}

void WorldTransform::Initialize() {
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = sizeof(TransformationMatrix);
    desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1;
    desc.SampleDesc.Count = 1; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    sDxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource_));
    resource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData_));

    matWorld = Mathematics::MakeIdentity4x4();
    if (mappedData_) {
        mappedData_->World = matWorld;
        mappedData_->WVP = matWorld;
    }
}

void WorldTransform::UpdateMatrix(const Matrix4x4& viewProjectionMatrix) {
    matWorld = Mathematics::MakeAffineMatrix(scale, rotation, translation);
    if (mappedData_) {
        mappedData_->World = matWorld;
        mappedData_->WVP = Mathematics::Multiply(matWorld, viewProjectionMatrix);
    }
}
