#include "3d/Object3d.h"
#include "3d/LightGroup.h"
#include "3d/ShadowMap.h"
#include "base/DirectXCommon.h"
#include "base/GraphicPipeline.h"
#include "3d/Model.h"
#include <algorithm>
#include <cmath>

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

Object3d::~Object3d() {
    if (materialResource_) {
        if (sDxCommon_) {
            sDxCommon_->DeferRelease(materialResource_);
        }
        materialResource_.Reset();
        materialData_ = nullptr;
    }
    if (bonePaletteResource_) {
        if (sDxCommon_) {
            sDxCommon_->DeferRelease(bonePaletteResource_);
        }
        bonePaletteResource_.Reset();
        mappedBonePalette_ = nullptr;
    }
}

void Object3d::Initialize() {
    if (!sDxCommon_ || sDxCommon_->IsDeviceLost()) {
        return;
    }
    if (materialResource_) {
        sDxCommon_->DeferRelease(materialResource_);
        materialResource_.Reset();
        materialData_ = nullptr;
    }
    if (bonePaletteResource_) {
        sDxCommon_->DeferRelease(bonePaletteResource_);
        bonePaletteResource_.Reset();
        mappedBonePalette_ = nullptr;
    }

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    // 1. Material Constant Buffer
    D3D12_RESOURCE_DESC matDesc{};
    matDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    matDesc.Width = (sizeof(Material) + 255) & ~255;
    matDesc.Height = 1; matDesc.DepthOrArraySize = 1; matDesc.MipLevels = 1;
    matDesc.SampleDesc.Count = 1; matDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    HRESULT hr = sDxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &matDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&materialResource_));
    if (FAILED(hr) || !materialResource_) {
        sDxCommon_->CheckDeviceLost(hr, "Object3d::Initialize(Material)");
        return;
    }
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    if (materialData_) {
        materialData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
        materialData_->enableLighting = 1;
        materialData_->lightingType = 4; // Blinn-Phong default
        materialData_->shininess = 32.0f;
        materialData_->specularColor = { 1.0f, 1.0f, 1.0f };
    }

    // 2. Bone Palette Constant Buffer (128 Matrix4x4 = 8192 bytes)
    D3D12_RESOURCE_DESC boneDesc{};
    boneDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    boneDesc.Width = sizeof(Matrix4x4) * 128;
    boneDesc.Height = 1; boneDesc.DepthOrArraySize = 1; boneDesc.MipLevels = 1;
    boneDesc.SampleDesc.Count = 1; boneDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    hr = sDxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &boneDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&bonePaletteResource_));
    if (FAILED(hr) || !bonePaletteResource_) {
        sDxCommon_->CheckDeviceLost(hr, "Object3d::Initialize(BonePalette)");
        return;
    }
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
        if (mappedBonePalette_) {
            for (int i = 0; i < 128; i++) {
                mappedBonePalette_[i] = Mathematics::MakeIdentity4x4();
            }
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
    if (!cmdList || !sDxCommon_ || sDxCommon_->IsDeviceLost() || !materialResource_ || worldTransform.GetGPUAddress() == 0) {
        return;
    }

    // Bind pipeline dengan blend mode yang sesuai untuk object ini
    if (sPipeline_) {
        GeometryShaderMode gsMode = geometryShaderMode;
        if (gsMode == GeometryShaderMode::None && useGeometryShader) {
            gsMode = GeometryShaderMode::PassThrough;
        }
        sPipeline_->Bind(cmdList, blendMode, gsMode);
    }

    cmdList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, worldTransform.GetGPUAddress());
    if (sLightGroup_) {
        sLightGroup_->Draw(cmdList, 3);
    }
    if (bonePaletteResource_) {
        cmdList->SetGraphicsRootConstantBufferView(4, bonePaletteResource_->GetGPUVirtualAddress());
    }
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
