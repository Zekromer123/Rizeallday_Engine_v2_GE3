#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <random>
#include <cstdint>
#include <algorithm>
#include "base/DirectXCommon.h"
#include "base/DXCManager.h"
#include "base/TextureManager.h"
#include "math/MathUtil.h"
#include "math/VertexData.h"
#include "3d/Camera.h"
#include "3d/Collider.h"

class ParticleQuad {
public:
	static constexpr uint32_t kMaxCapacity = 1024;

	struct Particle {
		Vector3 scale = { 0.25f, 0.25f, 0.25f };
		Vector3 startScale = { 0.25f, 0.25f, 0.25f };
		Vector3 endScale = { 0.25f, 0.25f, 0.25f };
		Vector3 rotate = { 0.0f, 0.0f, 0.0f };
		Vector3 translate = { 0.0f, 0.0f, 0.0f };
		Vector3 velocity = { 0.0f, 0.0f, 0.0f };
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 startColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 endColor = { 1.0f, 1.0f, 1.0f, 0.0f };
		float gravity = 0.0f;
		float drag = 0.0f;
		float lifeTime = 2.0f;
		float currentTime = 0.0f;
	};

	struct Emitter {
		Vector3 translate = { 0.0f, 0.0f, 0.0f };
		Vector3 radius = { 1.5f, 0.5f, 1.5f };
		uint32_t count = 3;
		float frequency = 0.15f;
		float frequencyTimer = 0.0f;
		bool emit = true;

		int shape = 1; // 0: Point, 1: Sphere, 2: Box, 3: Cone
		float spawnRadius = 0.5f;
		Vector3 boxExtents = { 1.0f, 1.0f, 1.0f };
		float coneAngle = 25.0f; // degrees
		Vector3 direction = { 0.0f, 1.0f, 0.0f };
		float spreadAngle = 30.0f; // degrees

		float speedMin = 1.0f;
		float speedMax = 3.0f;
		float gravity = 0.0f;
		float drag = 0.05f;

		float lifetimeMin = 1.5f;
		float lifetimeMax = 3.0f;

		float startScale = 0.25f;
		float endScale = 0.5f;

		Vector4 startColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 endColor = { 1.0f, 1.0f, 1.0f, 0.0f };

		int billboardMode = 0; // 0: Camera Facing, 1: Y-Axis Cylindrical, 2: Velocity Aligned
		float spawnRate = 20.0f; // particles per second
	};

	struct AccelerationField {
		Vector3 acceleration = { 15.0f, 0.0f, 0.0f };
		AABBCollider area = { { -30.0f, -10.0f, -30.0f }, { 30.0f, 30.0f, 30.0f } };
		bool enabled = false;
	};

	static constexpr uint32_t kMaxPasses = 2; // Pass 0 = Scene Viewport, Pass 1 = Game Viewport
	static constexpr uint32_t kPassScene = 0;
	static constexpr uint32_t kPassGame = 1;

	struct ParticleForGPU
	{
		Matrix4x4 WVP;
		Matrix4x4 World;
		Vector4 color;
	};

	struct ParticleCameraCB
	{
		Matrix4x4 viewProjection;
		Vector3 cameraPosition;
		float pad0;
		Vector3 billboardRight;
		float pad1;
		Vector3 billboardUp;
		float pad2;
	};

	enum class BlendType { Normal, Add };

	ParticleQuad() = default;
	~ParticleQuad();

	ParticleQuad(const ParticleQuad&) = delete;
	ParticleQuad& operator=(const ParticleQuad&) = delete;

	ParticleQuad(ParticleQuad&& other) noexcept;
	ParticleQuad& operator=(ParticleQuad&& other) noexcept;

	void Initialize(DirectXCommon* dxCommon, DXCManager* dxc, uint32_t textureHandle, uint32_t maxParticles = 100);
	void Simulate(float deltaTime);
	void PrepareForCamera(const Camera& camera, uint32_t passIndex = 0);
	void Update(float deltaTime, const Camera& camera);
	void Draw(ID3D12GraphicsCommandList* cmdList, uint32_t passIndex = 0);

	void EmitBurst(uint32_t count);
	void ApplyPreset(int presetIndex); // 1: Smoke, 2: Water Fountain, 3: Fireworks, 4: Explosion, 5: Torch
	void SetMaxParticles(uint32_t maxCount);
	void ClearParticles();

	bool useBillboard = true;
	bool isUpdate = true;
	BlendType blendType = BlendType::Normal;

	Emitter emitter;
	AccelerationField windField;
	Vector4 globalColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	uint32_t GetActiveCount() const { return static_cast<uint32_t>(particles_.size()); }
	uint32_t GetMaxParticles() const { return maxParticles_; }

private:
	void CreatePipeline(DXCManager* dxc);
	Vector4 GenerateRandomColor();
	Vector3 GenerateRandomVelocity();

	DirectXCommon* dxCommon_ = nullptr;
	uint32_t maxParticles_ = 100;
	uint32_t textureHandle_ = 0;
	uint32_t srvIndex_[kMaxPasses] = { 0, 0 };

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Material* materialData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_[kMaxPasses];
	ParticleForGPU* instancingData_[kMaxPasses] = { nullptr, nullptr };

	D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvGPU_[kMaxPasses]{};

	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
	uint8_t* cameraMappedData_ = nullptr;

	std::vector<Particle> particles_;
	std::mt19937 randomEngine_{ 12345 };
};
