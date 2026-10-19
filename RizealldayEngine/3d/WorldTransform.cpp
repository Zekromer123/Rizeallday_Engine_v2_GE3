#include "3d/WorldTransform.h"
#include "base/DirectXCommon.h"

DirectXCommon* WorldTransform::sDxCommon_ = nullptr;

void WorldTransform::StaticInitialize(DirectXCommon* dxCommon) {
    sDxCommon_ = dxCommon;
}

WorldTransform::~WorldTransform() {
    if (resource_) {
        if (sDxCommon_) {
            sDxCommon_->DeferRelease(resource_);
        }
        resource_.Reset();
        mappedData_ = nullptr;
    }
}

void WorldTransform::Initialize() {
    if (!sDxCommon_ || sDxCommon_->IsDeviceLost()) {
        return;
    }
    if (resource_ && sDxCommon_) {
        sDxCommon_->DeferRelease(resource_);
        resource_.Reset();
        mappedData_ = nullptr;
    }
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = (sizeof(TransformationMatrix) + 255) & ~255;
    desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1;
    desc.SampleDesc.Count = 1; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    HRESULT hr = sDxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource_));
    if (FAILED(hr) || !resource_) {
        sDxCommon_->CheckDeviceLost(hr, "WorldTransform::Initialize");
        return;
    }
    resource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData_));

    matWorld = Mathematics::MakeIdentity4x4();
    if (mappedData_) {
        mappedData_->World = matWorld;
        mappedData_->WVP = matWorld;
        mappedData_->WorldInverseTranspose = matWorld;
    }
}

void WorldTransform::UpdateMatrix(const Matrix4x4& viewProjectionMatrix) {
    Matrix4x4 scaleMatrix = Mathematics::MakeScaleMatrix(scale);
    Matrix4x4 rotMatrix = rotationQuat.ToRotationMatrix4x4();
    Matrix4x4 transMatrix = Mathematics::MakeTranslateMatrix(translation);
    matWorld = Mathematics::Multiply(scaleMatrix, Mathematics::Multiply(rotMatrix, transMatrix));
    if (mappedData_) {
        mappedData_->World = matWorld;
        mappedData_->WVP = Mathematics::Multiply(matWorld, viewProjectionMatrix);
        mappedData_->WorldInverseTranspose = Mathematics::Transpose(Mathematics::Inverse(matWorld));
    }
}
