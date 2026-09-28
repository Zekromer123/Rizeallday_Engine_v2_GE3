#include "3d/PrimitiveDrawer.h"
#include "base/DirectXCommon.h"
#include <cassert>
#include <algorithm>

PrimitiveDrawer* PrimitiveDrawer::GetInstance()
{
	static PrimitiveDrawer instance;
	return &instance;
}

void PrimitiveDrawer::Initialize(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	size_t sizeInBytes = sizeof(LineVertex) * kMaxVertices;
	vertexResource_.Attach(dxCommon_->CreateBufferResource(sizeInBytes));
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertices_));

	vbView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vbView_.SizeInBytes = static_cast<UINT>(sizeInBytes);
	vbView_.StrideInBytes = sizeof(LineVertex);

	lineVertices_.reserve(kMaxVertices);
}

void PrimitiveDrawer::DrawLine3d(const Vector3& p1, const Vector3& p2, const Vector4& color)
{
	if (lineVertices_.size() + 2 > kMaxVertices) return;

	lineVertices_.push_back({ { p1.x, p1.y, p1.z, 1.0f }, color });
	lineVertices_.push_back({ { p2.x, p2.y, p2.z, 1.0f }, color });
}

void PrimitiveDrawer::DrawGrid(float size, float subdivisions, const Vector4& color)
{
	float halfSize = size * 0.5f;
	float step = size / subdivisions;

	for (float i = -halfSize; i <= halfSize; i += step) {
		DrawLine3d({ i, 0.0f, -halfSize }, { i, 0.0f, halfSize }, color);
		DrawLine3d({ -halfSize, 0.0f, i }, { halfSize, 0.0f, i }, color);
	}
}

void PrimitiveDrawer::Render(ID3D12GraphicsCommandList* cmdList)
{
	if (lineVertices_.empty()) return;

	size_t count = (std::min)(lineVertices_.size(), kMaxVertices);
	memcpy(mappedVertices_, lineVertices_.data(), sizeof(LineVertex) * count);

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView_);
	cmdList->DrawInstanced(static_cast<UINT>(count), 1, 0, 0);

	lineVertices_.clear();
}

void PrimitiveDrawer::Reset()
{
	lineVertices_.clear();
}
