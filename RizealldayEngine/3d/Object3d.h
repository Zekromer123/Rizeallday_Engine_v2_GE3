#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include "3d/WorldTransform.h"
#include "3d/Camera.h"
#include "math/VertexData.h"
#include "base/BlendMode.h"

class Model;
class DirectXCommon;
class GraphicPipeline;

class Object3d {
public:
	static void StaticInitialize(DirectXCommon* dxCommon);
	static void StaticSetPipeline(GraphicPipeline* p);

	void Initialize();
	void Update(float deltaTime = 1.0f / 60.0f);
	void Draw(ID3D12GraphicsCommandList* cmdList);

	void SetModel(Model* model, bool resetAnim = true);
	Model* GetModel() const { return model_; }
	void SetCamera(Camera* camera) { camera_ = camera; }

	// Animation controls
	void PlayAnimation(int animIndex, bool loop = true, float speed = 1.0f);
	void SetAnimationTime(float time);
	float GetAnimationTime() const { return animationTime_; }
	float GetAnimationDuration() const;
	void SetAnimationSpeed(float speed) { animationSpeed_ = speed; }
	float GetAnimationSpeed() const { return animationSpeed_; }
	void SetAnimationLoop(bool loop) { isAnimationLoop_ = loop; }
	bool IsAnimationLoop() const { return isAnimationLoop_; }

	/// Blend mode untuk rendering (default: Opaque)
	BlendMode blendMode = BlendMode::Opaque;

	/// Setter / Getter blend mode
	void SetBlendMode(BlendMode mode) { blendMode = mode; }
	BlendMode GetBlendMode() const { return blendMode; }

	/// Akses material untuk mengubah warna/alpha
	Material* GetMaterial() const { return materialData_; }

	WorldTransform worldTransform;

private:
	static DirectXCommon* sDxCommon_;
	static GraphicPipeline* sPipeline_;

	Model* model_ = nullptr;
	Camera* camera_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Material* materialData_ = nullptr;

	// Bone Palette constant buffer (128 matrices * 64 bytes = 8192 bytes)
	Microsoft::WRL::ComPtr<ID3D12Resource> bonePaletteResource_;
	Matrix4x4* mappedBonePalette_ = nullptr;
	std::vector<Matrix4x4> boneTransforms_;

	float animationTime_ = 0.0f;
	float animationSpeed_ = 1.0f;
	bool isAnimationLoop_ = true;
	int currentAnimationIndex_ = 0;
};
