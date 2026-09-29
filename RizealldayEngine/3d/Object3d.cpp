#include "3d/Object3d.h"
#include "base/DirectXCommon.h"
#include "base/GraphicPipeline.h"
#include "3d/Model.h"
#include <algorithm>
#include <cmath>

DirectXCommon* Object3d::sDxCommon_ = nullptr;
GraphicPipeline* Object3d::sPipeline_ = nullptr;

void Object3d::StaticInitialize(DirectXCommon* dxCommon) {
    sDxCommon_ = dxCommon;
}

void Object3d::StaticSetPipeline(GraphicPipeline* p) {
    sPipeline_ = p;
}

void Object3d::Initialize() {
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    // 1. Material Constant Buffer
    D3D12_RESOURCE_DESC matDesc{};
    matDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    matDesc.Width = sizeof(Material);
    matDesc.Height = 1; matDesc.DepthOrArraySize = 1; matDesc.MipLevels = 1;
    matDesc.SampleDesc.Count = 1; matDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    sDxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &matDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&materialResource_));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    if (materialData_) {
        materialData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
        materialData_->enableLighting = 1;
        materialData_->lightingType = 2;
    }

    // 2. Bone Palette Constant Buffer (128 Matrix4x4 = 8192 bytes)
    D3D12_RESOURCE_DESC boneDesc{};
    boneDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    boneDesc.Width = sizeof(Matrix4x4) * 128;
    boneDesc.Height = 1; boneDesc.DepthOrArraySize = 1; boneDesc.MipLevels = 1;
    boneDesc.SampleDesc.Count = 1; boneDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    sDxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &boneDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&bonePaletteResource_));
    bonePaletteResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedBonePalette_));

    if (mappedBonePalette_) {
        for (int i = 0; i < 128; i++) {
            mappedBonePalette_[i] = Mathematics::MakeIdentity4x4();
        }
    }

    // 3. World Transform
    worldTransform.Initialize();
}

void Object3d::SetModel(Model* model, bool resetAnim) {
    if (model_ != model) {
        model_ = model;
        if (resetAnim) {
            animationTime_ = 0.0f;
        }
    }
}

void Object3d::PlayAnimation(int animIndex, bool loop, float speed) {
    currentAnimationIndex_ = animIndex;
    isAnimationLoop_ = loop;
    animationSpeed_ = speed;
    animationTime_ = 0.0f;
}

void Object3d::SetAnimationTime(float time) {
    animationTime_ = time;
}

float Object3d::GetAnimationDuration() const {
    if (model_) {
        return model_->GetAnimationDuration(currentAnimationIndex_);
    }
    return 0.0f;
}

void Object3d::Update(float deltaTime) {
    if (camera_) {
        worldTransform.UpdateMatrix(camera_->GetViewProjectionMatrix());
    } else {
        worldTransform.UpdateMatrix(Mathematics::MakeIdentity4x4());
    }

    if (model_ && model_->HasBones() && mappedBonePalette_) {
        animationTime_ += deltaTime * animationSpeed_;
        float duration = model_->GetAnimationDuration(currentAnimationIndex_);
        if (duration > 0.0f) {
            if (isAnimationLoop_) {
                animationTime_ = std::fmod(animationTime_, duration);
            } else if (animationTime_ > duration) {
                animationTime_ = duration;
            }
        }

        model_->CalculateBoneTransforms(animationTime_, currentAnimationIndex_, boneTransforms_);
        size_t copyCount = (std::min)(boneTransforms_.size(), static_cast<size_t>(128));
        std::memcpy(mappedBonePalette_, boneTransforms_.data(), sizeof(Matrix4x4) * copyCount);
    }
}

void Object3d::Draw(ID3D12GraphicsCommandList* cmdList) {
    // Bind pipeline dengan blend mode yang sesuai untuk object ini
    if (sPipeline_) {
        sPipeline_->Bind(cmdList, blendMode);
    }

    cmdList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, worldTransform.GetGPUAddress());
    if (bonePaletteResource_) {
        cmdList->SetGraphicsRootConstantBufferView(4, bonePaletteResource_->GetGPUVirtualAddress());
    }
    if (model_) {
        model_->Draw(cmdList);
    }
}
