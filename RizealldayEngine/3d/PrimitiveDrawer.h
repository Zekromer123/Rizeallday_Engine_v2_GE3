#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"

class DirectXCommon;
class DXCManager;
class Camera;

/// Immediate-mode 3D line renderer (editor grid, debug lines).
/// Lines are depth-tested against the scene but do not write depth.
class PrimitiveDrawer
{
public:
	struct LineVertex {
		Vector4 position;
		Vector4 color;
	};

	static PrimitiveDrawer* GetInstance();

	void Initialize(DirectXCommon* dxCommon, DXCManager* dxc);
	void Finalize();

	/// Draw 3D line (queue for rendering)
	void DrawLine3d(const Vector3& p1, const Vector3& p2, const Vector4& color);

	/// Draw grid centered at the origin (queue for rendering)
	void DrawGrid(float size = 10.0f, float subdivisions = 10.0f, const Vector4& color = { 0.5f, 0.5f, 0.5f, 1.0f });

	/// Editor ground grid on the XZ plane around `center`, fading out towards `halfExtent`.
	/// The X axis line (z = 0) and Z axis line (x = 0) are tinted when they fall inside the grid.
	void DrawEditorGrid(const Vector3& center, int halfExtent, const Vector4& color, const Vector4& xAxisColor, const Vector4& zAxisColor);

	/// Render queued primitives with the given camera into the currently bound render target, then clear the queue
	void Render(ID3D12GraphicsCommandList* cmdList, const Camera& camera);

	/// Reset per-frame primitive buffer
	void Reset();

private:
	PrimitiveDrawer() = default;
	~PrimitiveDrawer() = default;
	PrimitiveDrawer(const PrimitiveDrawer&) = delete;
	PrimitiveDrawer& operator=(const PrimitiveDrawer&) = delete;

	void CreateRootSignature();
	void CreatePipeline(DXCManager* dxc);
	void PushLine(const Vector3& p1, const Vector4& c1, const Vector3& p2, const Vector4& c2);

	static const size_t kMaxVertices = 16384;

	DirectXCommon* dxCommon_ = nullptr;

	std::vector<LineVertex> lineVertices_;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;
};
