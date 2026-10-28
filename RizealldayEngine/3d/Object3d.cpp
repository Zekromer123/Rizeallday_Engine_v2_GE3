#include "3d/Object3d.h"
#include "3d/Camera.h"
#include "3d/LightGroup.h"
#include "3d/ShadowMap.h"
#include "base/DirectXCommon.h"
#include "base/GraphicPipeline.h"
#include "3d/Model.h"
#include <algorithm>
#include <cmath>
#include <cstring>

DirectXCommon* Object3d::sDxCommon_ = nullptr;
GraphicPipeline* Object3d::sPipeline_ = nullptr;
LightGroup* Object3d::sLightGroup_ = nullptr;
ShadowMap* Object3d::sShadowMap_ = nullptr;

void Object3d::StaticInitialize(DirectXCommon* dxCommon) {
    sDxCommon_ = dxCommon;
}

void Object3d::StaticSetPipeline(GraphicPipeline* p) {
    sPipeline_ = p;
}

void Object3d::StaticSetLightGroup(LightGroup* lights) {
    sLightGroup_ = lights;
}

void Object3d::StaticSetShadowMap(ShadowMap* shadowMap) {
    sShadowMap_ = shadowMap;
}

Object3d::~Object3d() = default;

void Object3d::Initialize() {
    // 1. Material (CPU copy, uploaded per draw)
    material_ = Material{};
    material_.color = {1.0f, 1.0f, 1.0f, 1.0f};
    material_.enableLighting = 1;
    material_.lightingType = 4; // Blinn-Phong default
    material_.shininess = 32.0f;
    material_.specularColor = { 1.0f, 1.0f, 1.0f };

    // 2. Bone palette (identity until an animated model updates it)
    ResetBonePalette();

    // 3. World Transform
    worldTransform.Initialize();
}

void Object3d::ResetBonePalette() {
    bonePalette_.assign(kMaxBones, Mathematics::MakeIdentity4x4());
}

void Object3d::SetModel(Model* model, bool resetAnim) {
    if (model_ != model) {
        model_ = model;
        if (resetAnim) {
            animationTime_ = 0.0f;
        }
        ResetBonePalette();
    }
}

void Object3d::PlayAnimation(int animIndex, bool loop, float speed) {
    currentAnimationIndex_ = animIndex;
    isAnimationLoop_ = loop;
    animationSpeed_ = speed;
    animationTime_ = 0.0f;
}

void Object3d::SetCurrentAnimationIndex(int animIndex) {
    if (currentAnimationIndex_ != animIndex) {
        currentAnimationIndex_ = animIndex;
        animationTime_ = 0.0f;
    }
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

void Object3d::CopyBonePaletteFrom(const Object3d& other) {
    animationTime_ = other.animationTime_;
    currentAnimationIndex_ = other.currentAnimationIndex_;
    animationSpeed_ = other.animationSpeed_;
    isAnimationLoop_ = other.isAnimationLoop_;
    boneTransforms_ = other.boneTransforms_;
    bonePalette_ = other.bonePalette_;
}

void Object3d::Update(float deltaTime) {
    if (camera_) {
        worldTransform.UpdateMatrix(camera_->GetViewProjectionMatrix());
    } else {
        worldTransform.UpdateMatrix(Mathematics::MakeIdentity4x4());
    }

    if (model_ && model_->HasBones()) {
        animationTime_ += deltaTime * animationSpeed_;
        float duration = model_->GetAnimationDuration(currentAnimationIndex_);
        if (duration > 0.0f) {
            if (isAnimationLoop_) {
                animationTime_ = std::fmod(animationTime_, duration);
                if (animationTime_ < 0.0f) {
                    animationTime_ += duration;
                }
            } else if (animationTime_ > duration) {
                animationTime_ = duration;
            }
        }
        model_->CalculateBoneTransforms(animationTime_, currentAnimationIndex_, boneTransforms_);

        if (bonePalette_.size() != kMaxBones) {
            ResetBonePalette();
        }
        size_t copyCount = (std::min)(boneTransforms_.size(), kMaxBones);
        std::memcpy(bonePalette_.data(), boneTransforms_.data(), sizeof(Matrix4x4) * copyCount);
    }
}

D3D12_GPU_VIRTUAL_ADDRESS Object3d::UploadBonePalette() const {
    if (!sDxCommon_ || sDxCommon_->IsDeviceLost()) {
        return 0;
    }
    const size_t paletteBytes = sizeof(Matrix4x4) * kMaxBones;
    if (model_ && model_->HasBones() && bonePalette_.size() == kMaxBones) {
        return sDxCommon_->UploadData(bonePalette_.data(), paletteBytes);
    }

    // Shared identity palette, uploaded once per frame for all static meshes
    static uint64_t sIdentityFrame = ~0ull;
    static D3D12_GPU_VIRTUAL_ADDRESS sIdentityAddress = 0;
    if (sIdentityFrame != sDxCommon_->GetFrameIndex() || sIdentityAddress == 0) {
        DirectXCommon::UploadAllocation alloc = sDxCommon_->AllocateUpload(paletteBytes);
        if (!alloc.cpu) return 0;
        Matrix4x4* dst = static_cast<Matrix4x4*>(alloc.cpu);
        const Matrix4x4 identity = Mathematics::MakeIdentity4x4();
        for (size_t i = 0; i < kMaxBones; ++i) {
            dst[i] = identity;
        }
        sIdentityAddress = alloc.gpu;
        sIdentityFrame = sDxCommon_->GetFrameIndex();
    }
    return sIdentityAddress;
}

void Object3d::Draw(ID3D12GraphicsCommandList* cmdList) {
    if (!cmdList || !sDxCommon_ || sDxCommon_->IsDeviceLost()) {
        return;
    }

    const D3D12_GPU_VIRTUAL_ADDRESS materialAddr = sDxCommon_->UploadConstants(material_);
    const D3D12_GPU_VIRTUAL_ADDRESS transformAddr = worldTransform.Upload();
    const D3D12_GPU_VIRTUAL_ADDRESS boneAddr = UploadBonePalette();
    if (materialAddr == 0 || transformAddr == 0 || boneAddr == 0) {
        return;
    }

    // Bind pipeline with matching blend mode for this object
    if (sPipeline_) {
        GeometryShaderMode gsMode = geometryShaderMode;
        if (gsMode == GeometryShaderMode::None && useGeometryShader) {
            gsMode = GeometryShaderMode::PassThrough;
        }
        sPipeline_->Bind(cmdList, blendMode, gsMode);
    }

    cmdList->SetGraphicsRootConstantBufferView(0, materialAddr);
    cmdList->SetGraphicsRootConstantBufferView(1, transformAddr);

    if (sLightGroup_) {
        sLightGroup_->Draw(cmdList, 3);
    }

    cmdList->SetGraphicsRootConstantBufferView(4, boneAddr);

    D3D12_GPU_VIRTUAL_ADDRESS camAddr = camera_ ? camera_->GetGPUVirtualAddress() : Camera::GetDefaultGPUVirtualAddress();
    if (camAddr != 0) {
        cmdList->SetGraphicsRootConstantBufferView(5, camAddr);
    }

    if (sShadowMap_) {
        cmdList->SetGraphicsRootDescriptorTable(6, sShadowMap_->GetSrvGPUHandle());
    }

    if (model_) {
        model_->Draw(cmdList, textureHandle_);
    }
}
