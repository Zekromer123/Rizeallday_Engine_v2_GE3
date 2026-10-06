#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <random>
#include <cstdint>
#include "base/DirectXCommon.h"
#include "base/DXCManager.h"
#include "base/TextureManager.h"
#include "math/MathUtil.h"
#include "math/VertexData.h"
#include "3d/Camera.h"
#include "3d/Collider.h"

class ParticleQuad {
public:
	struct Particle {
		Vector3 scale = { 1.0f,1.0f,1.0f };
		Vector3 rotate = { 0.0f,0.0f,0.0f };
		Vector3 translate = { 0.0f,0.0f,0.0f };
		Vector3 velocity = { 0.0f,0.0f,0.0f };
		Vector4 color = { 1.0f,1.0f,1.0f,1.0f };
		float lifeTime = 2.0f;
		float currentTime = 0.0f;
	};

	struct Emitter {
		Vector3 translate = { 0.0f,0.0f,0.0f };
		Vector3 radius = { 1.5f,0.5f,1.5f };
		uint32_t count = 3;
		float frequency = 0.15f;
		float frequencyTimer = 0.0f;
		bool emit = true;
	};

	struct AccelerationField {
		Vector3 acceleration = { 15.0f,0.0f,0.0f };
		AABBCollider area = { { -30.0f, -10.0f, -30.0f }, { 30.0f, 30.0f, 30.0f } };
		bool enabled = false;
	};

	struct ParticleForGPU
	{
		Matrix4x4 WVP;
		Matrix4x4 World;
		Vector4 color;
	};

	enum class BlendType { Normal, Add };

	void Initialize(DirectXCommon* dxCommon, DXCManager* dxc, uint32_t textureHandle, uint32_t maxParticles = 100);
	void Update(float deltaTime,const Camera& camera);
	void Draw(ID3D12GraphicsCommandList* cmdList);

	void EmitBurst(uint32_t count);

	bool useBillboard = true;
	bool isUpdate = true;
	BlendType blendType = BlendType::Add;

	Emitter emitter;
	AccelerationField windField;
	Vector4 globalColor = {1.0f,1.0f,1.0f,1.0f};

	uint32_t GetActiveCount() const { return static_cast<uint32_t>(particles_.size()); };
	uint32_t GetMaxParticles() const { return maxParticles_; };
private:
	void CreatePipeline(DXCManager* dxc);
	Vector4 GenerateRandomColor();
	Vector3 GenerateRandomVelocity();

	DirectXCommon* dxCommon_ = nullptr;
	uint32_t maxParticles_ = 100;
	uint32_t textureHandle_ = 0;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> psoNormal_; //AlphaBlend
	Microsoft::WRL::ComPtr<ID3D12PipelineState> psoAdd_; //Additive glow

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Material* materialData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_;
	ParticleForGPU* instancingData_ = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvGPU_{};

	std::vector<Particle> particles_;
	//std::random_device seedGenerator;
	//std::mt19937 randomEngine_{ seedGenerator() };
	// same as rand() to generate random
	std::mt19937 randomEngine_{ 12345 };
};
