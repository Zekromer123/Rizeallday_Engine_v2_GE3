#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cassert>
#include "base/DirectXCommon.h"
#include "math/MathUtil.h"
#include "math/VertexData.h"
#include "3d/DirectionalLight.h"

class Sprite
{
public:
	void Initialize(DirectXCommon* dxCommon);
	void Update();
	void Draw(ID3D12GraphicsCommandList* commandList);
	void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE handle) { textureSrvHandleGPUSprite = handle; }
	void Finalize()
	{
		if (vertexResourceSprite) {
			vertexResourceSprite->Unmap(0, nullptr);
		}
		if (transformationMatrixResourceSprite) {
			transformationMatrixResourceSprite->Unmap(0, nullptr);
		}
		if (materialResourceSprite) {
			materialResourceSprite->Unmap(0, nullptr);
		}
		if (indexResourceSprite) {
			indexResourceSprite->Unmap(0, nullptr);
		}
		if (directionalLightResource) {
			directionalLightResource->Unmap(0, nullptr);
		}
	}

public:
	Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	Material* materialDataSprite = nullptr;
	DirectionalLightData* directionalLightData = nullptr;

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite = nullptr;
	TransformationMatrix* transformationMatrixDataSprite = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
	ID3D12DescriptorHeap* srvDescriptorHeap = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPUSprite{};
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite = nullptr;
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};

	const float kClientWidth = 1280.0f;
	const float kClientHeight = 720.0f;
};
