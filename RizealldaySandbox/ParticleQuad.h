#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <cstdint>
#include "base/DirectXCommon.h"
#include "base/DXCManager.h"
#include "base/TextureManager.h"
#include "math/MathUtil.h"
#include "math/VertexData.h"

class ParticleQuad {
public:
	struct Transform {
		Vector3 scale = { 1.0f, 1.0f, 1.0f };
		Vector3 rotate = { 0.0f, 0.0f, 0.0f };
		Vector3 translate = { 0.0f, 0.0f, 0.0f };
	};

	void Initialize(DirectXCommon* dxCommon, DXCManager* dxc, uint32_t textureHandle, uint32_t numInstances = 10);
	void Update(const Matrix4x4& viewProjectionMatrix);
	void Draw(ID3D12GraphicsCommandList* cmdList);

	std::vector<Transform>& GetTransforms() { return transforms_; }
	uint32_t GetNumInstances() const { return numInstances_; }
	void SetTextureHandle(uint32_t handle) { textureHandle_ = handle; }

	Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

private:
	void CreatePipeline(DXCManager* dxc);

	DirectXCommon* dxCommon_ = nullptr;
	uint32_t numInstances_ = 10;
	uint32_t textureHandle_ = 0;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Material* materialData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_;
	TransformationMatrix* instancingData_ = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvGPU_{};

	std::vector<Transform> transforms_;
};
