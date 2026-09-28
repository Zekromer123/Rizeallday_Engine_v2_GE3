#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "3d/WorldTransform.h"
#include "3d/Camera.h"
#include "math/VertexData.h"

class Model;
class DirectXCommon;

class Object3d {
public:
	static void StaticInitialize(DirectXCommon* dxCommon);
	void Initialize();
	void Update();
	void Draw(ID3D12GraphicsCommandList* cmdList);

	void SetModel(Model* model) { model_ = model; }
	void SetCamera(Camera* camera) { camera_ = camera; }

	WorldTransform worldTransform;

private:
	static DirectXCommon* sDxCommon_;
	Model* model_ = nullptr;
	Camera* camera_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Material* materialData_ = nullptr;
};
