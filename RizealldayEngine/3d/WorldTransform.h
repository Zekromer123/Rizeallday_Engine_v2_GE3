#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "math/Vector3.h"
#include "math/Matrix4x4.h"
#include "math/MathUtil.h"

class DirectXCommon;

class WorldTransform {
public:
	static void StaticInitialize(DirectXCommon* dxCommon);
	void Initialize();
	void UpdateMatrix(const Matrix4x4& viewProjectionMatrix);
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const { return resource_->GetGPUVirtualAddress(); }

	Vector3 scale = {1.0f, 1.0f, 1.0f};
	Vector3 rotation = {0.0f, 0.0f, 0.0f};
	Vector3 translation = {0.0f, 0.0f, 0.0f};
	Matrix4x4 matWorld = {};

private:
	static DirectXCommon* sDxCommon_;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
	TransformationMatrix* mappedData_ = nullptr;
};
