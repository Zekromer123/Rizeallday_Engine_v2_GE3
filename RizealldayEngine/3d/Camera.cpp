#include "3d/Camera.h"
#include "math/MathUtil.h"
#include "base/DirectXCommon.h"

DirectXCommon* Camera::sDxCommon_ = nullptr;
Microsoft::WRL::ComPtr<ID3D12Resource> Camera::sDefaultCameraResource_;
CameraForGPU* Camera::sDefaultMappedData_ = nullptr;

void Camera::StaticInitialize(DirectXCommon* dxCommon) {
    sDxCommon_ = dxCommon;
    if (sDxCommon_ && !sDefaultCameraResource_) {
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = (sizeof(CameraForGPU) + 255) & ~255;
        desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1;
        desc.SampleDesc.Count = 1; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        sDxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&sDefaultCameraResource_));
        if (sDefaultCameraResource_) {
            sDefaultCameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&sDefaultMappedData_));
            if (sDefaultMappedData_) {
                sDefaultMappedData_->worldPosition = { 0.0f, 0.0f, -10.0f };
                sDefaultMappedData_->pad = 0.0f;
            }
        }
    }
}

D3D12_GPU_VIRTUAL_ADDRESS Camera::GetDefaultGPUVirtualAddress() {
    return sDefaultCameraResource_ ? sDefaultCameraResource_->GetGPUVirtualAddress() : 0;
}

Camera::~Camera() {
    if (cameraResource_) {
        cameraResource_->Unmap(0, nullptr);
        mappedCameraData_ = nullptr;
        if (sDxCommon_) {
            sDxCommon_->DeferRelease(cameraResource_);
        }
        cameraResource_.Reset();
    }
}

void Camera::CreateBufferIfNeeded() {
    if (!cameraResource_ && sDxCommon_) {
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = (sizeof(CameraForGPU) + 255) & ~255;
        desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1;
        desc.SampleDesc.Count = 1; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        sDxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cameraResource_));
        if (cameraResource_) {
            cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedCameraData_));
        }
    }
}

D3D12_GPU_VIRTUAL_ADDRESS Camera::GetGPUVirtualAddress() const {
    if (cameraResource_) {
        return cameraResource_->GetGPUVirtualAddress();
    }
    return GetDefaultGPUVirtualAddress();
}

void Camera::Initialize() {
    CreateBufferIfNeeded();
    matView_ = Mathematics::MakeIdentity4x4();
    matProjection_ = Mathematics::MakePerspectiveFovMatrix(fovY, aspectRatio, nearClip, farClip);
    matViewProjection_ = Mathematics::Multiply(matView_, matProjection_);
    if (mappedCameraData_) {
        mappedCameraData_->worldPosition = translation;
        mappedCameraData_->pad = 0.0f;
    }
}

void Camera::Update() {
    CreateBufferIfNeeded();
    Matrix4x4 cameraWorldMatrix = Mathematics::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, rotation, translation);
    matView_ = Mathematics::Inverse(cameraWorldMatrix);
    matProjection_ = Mathematics::MakePerspectiveFovMatrix(fovY, aspectRatio, nearClip, farClip);
    matViewProjection_ = Mathematics::Multiply(matView_, matProjection_);
    if (mappedCameraData_) {
        mappedCameraData_->worldPosition = translation;
        mappedCameraData_->pad = 0.0f;
    }
}
