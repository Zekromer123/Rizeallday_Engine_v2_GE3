#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"

class DirectXCommon;
class Camera;

class PrimitiveDrawer
{
public:
	struct LineVertex {
		Vector4 position;
		Vector4 color;
	};

	static PrimitiveDrawer* GetInstance();

	void Initialize(DirectXCommon* dxCommon);
	void SetCamera(Camera* camera) { camera_ = camera; }

	/// Draw 3D line (queue for rendering)
	void DrawLine3d(const Vector3& p1, const Vector3& p2, const Vector4& color);

	/// Draw grid (queue for rendering)
	void DrawGrid(float size = 10.0f, float subdivisions = 10.0f, const Vector4& color = { 0.5f, 0.5f, 0.5f, 1.0f });

	/// Render queued primitives and clear buffer
	void Render(ID3D12GraphicsCommandList* cmdList);

	/// Reset per-frame primitive buffer
	void Reset();

private:
	PrimitiveDrawer() = default;
	~PrimitiveDrawer() = default;
	PrimitiveDrawer(const PrimitiveDrawer&) = delete;
	PrimitiveDrawer& operator=(const PrimitiveDrawer&) = delete;

	static const size_t kMaxVertices = 4096;

	DirectXCommon* dxCommon_ = nullptr;
	Camera* camera_ = nullptr;

	std::vector<LineVertex> lineVertices_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vbView_{};
	LineVertex* mappedVertices_ = nullptr;
};
