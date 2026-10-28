#include "3d/WorldTransform.h"
#include "base/DirectXCommon.h"

DirectXCommon* WorldTransform::sDxCommon_ = nullptr;

void WorldTransform::StaticInitialize(DirectXCommon* dxCommon) {
    sDxCommon_ = dxCommon;
}

void WorldTransform::Initialize() {
    matWorld = Mathematics::MakeIdentity4x4();
    data_.World = matWorld;
    data_.WVP = matWorld;
    data_.WorldInverseTranspose = matWorld;
}

void WorldTransform::UpdateMatrix(const Matrix4x4& viewProjectionMatrix) {
    Matrix4x4 scaleMatrix = Mathematics::MakeScaleMatrix(scale);
    Matrix4x4 rotMatrix = rotationQuat.ToRotationMatrix4x4();
    Matrix4x4 transMatrix = Mathematics::MakeTranslateMatrix(translation);
    matWorld = Mathematics::Multiply(scaleMatrix, Mathematics::Multiply(rotMatrix, transMatrix));

    data_.World = matWorld;
    data_.WVP = Mathematics::Multiply(matWorld, viewProjectionMatrix);
    data_.WorldInverseTranspose = Mathematics::Transpose(Mathematics::Inverse(matWorld));
}

D3D12_GPU_VIRTUAL_ADDRESS WorldTransform::Upload() const {
    if (!sDxCommon_ || sDxCommon_->IsDeviceLost()) {
        return 0;
    }
    return sDxCommon_->UploadConstants(data_);
}
