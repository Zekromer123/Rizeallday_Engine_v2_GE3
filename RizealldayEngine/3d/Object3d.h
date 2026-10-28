#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include "3d/WorldTransform.h"
#include "math/VertexData.h"
#include "base/BlendMode.h"
#include "base/GeometryShaderMode.h"

class Camera;
class GraphicPipeline;
class Model;
class DirectXCommon;
class LightGroup;
class ShadowMap;

class Object3d {
public:
	Object3d() = default;
	~Object3d();
	static void StaticInitialize(DirectXCommon* dxCommon);
	static void StaticSetPipeline(GraphicPipeline* p);
	static void StaticSetLightGroup(LightGroup* lights);
	static void StaticSetShadowMap(ShadowMap* shadowMap);

	void Initialize();
	void Update(float deltaTime = 1.0f / 60.0f);
	void Draw(ID3D12GraphicsCommandList* cmdList);

	void SetModel(Model* model, bool resetAnim = true);
	Model* GetModel() const { return model_; }
	void SetCamera(Camera* camera) { camera_ = camera; }

	// Animation controls
	void PlayAnimation(int animIndex, bool loop = true, float speed = 1.0f);
	void SetCurrentAnimationIndex(int animIndex);
	int GetCurrentAnimationIndex() const { return currentAnimationIndex_; }
	void SetAnimationTime(float time);
	void ResetAnimationTime() { animationTime_ = 0.0f; }
	float GetAnimationTime() const { return animationTime_; }
	float GetAnimationDuration() const;
	void SetAnimationSpeed(float speed) { animationSpeed_ = speed; }
	float GetAnimationSpeed() const { return animationSpeed_; }
	void SetAnimationLoop(bool loop) { isAnimationLoop_ = loop; }
	void SetLoopAnimation(bool loop) { isAnimationLoop_ = loop; }
	bool IsAnimationLoop() const { return isAnimationLoop_; }
	void CopyBonePaletteFrom(const Object3d& other);
	/// Uploads this frame's bone palette (b1 VS, 128 matrices) and returns its GPU address.
	/// Meshes without bones share one identity palette per frame instead of uploading 8 KB each.
	D3D12_GPU_VIRTUAL_ADDRESS UploadBonePalette() const;

	/// Blend mode for rendering (default: Opaque)
	BlendMode blendMode = BlendMode::Opaque;

	/// Setter / Getter blend mode
	void SetBlendMode(BlendMode mode) { blendMode = mode; }
	BlendMode GetBlendMode() const { return blendMode; }

	bool useGeometryShader = false;
	void SetUseGeometryShader(bool use) { useGeometryShader = use; }
	bool GetUseGeometryShader() const { return useGeometryShader; }

	GeometryShaderMode geometryShaderMode = GeometryShaderMode::None;
	void SetGeometryShaderMode(GeometryShaderMode mode) { geometryShaderMode = mode; }
	GeometryShaderMode GetGeometryShaderMode() const { return geometryShaderMode; }

	void SetTextureHandle(uint32_t handle) { textureHandle_ = handle; }
	uint32_t GetTextureHandle() const { return textureHandle_; }

	/// Access material to modify color/alpha
	/// (CPU copy; uploaded for each draw, so changes apply to the next Draw call)
	Material* GetMaterial() { return &material_; }

	WorldTransform worldTransform;

private:
	static DirectXCommon* sDxCommon_;
	static GraphicPipeline* sPipeline_;
	static LightGroup* sLightGroup_;
	static ShadowMap* sShadowMap_;

	Model* model_ = nullptr;
	Camera* camera_ = nullptr;
	uint32_t textureHandle_ = 0;
	// Per-draw constants are kept on the CPU and uploaded through DirectXCommon's per-frame allocator
	Material material_{};

	// Bone palette (128 matrices * 64 bytes = 8192 bytes); only uploaded for meshes that have bones
	static constexpr size_t kMaxBones = 128;
	std::vector<Matrix4x4> bonePalette_;
	std::vector<Matrix4x4> boneTransforms_;
	void ResetBonePalette();

	float animationTime_ = 0.0f;
	float animationSpeed_ = 1.0f;
	bool isAnimationLoop_ = true;
	int currentAnimationIndex_ = 0;
};
