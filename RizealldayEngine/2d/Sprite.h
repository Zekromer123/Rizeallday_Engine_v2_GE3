#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cassert>
#include <cstdint>
#include <string>
#include "math/MathUtil.h"
#include "math/VertexData.h"
#include "3d/DirectionalLight.h"

class DirectXCommon;


class Sprite
{
public:
	Sprite() = default;
	~Sprite();

	void Initialize(DirectXCommon* dxCommon, const std::string& textureFilePath = "");
	void Initialize(DirectXCommon* dxCommon, uint32_t textureHandle);
	void Update();
	void Draw(ID3D12GraphicsCommandList* commandList);
	void Finalize();

	/// Bind texture by GPU descriptor handle
	void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE handle);

	/// Bind texture by TextureManager handle ID
	void SetTextureHandle(uint32_t textureHandle);
	uint32_t GetTextureHandle() const { return textureHandle_; }

	/// Texture cutout rectangle (top-left pixel coordinate and cutout width/height in pixels)
	const Vector2& GetTextureLeftTop() const { return textureLeftTop_; }
	void SetTextureLeftTop(const Vector2& textureLeftTop);

	const Vector2& GetTextureSize() const { return textureSize_; }
	void SetTextureSize(const Vector2& textureSize);

	void SetTextureRect(const Vector2& textureLeftTop, const Vector2& textureSize);
	void SetTextureRect(float left, float top, float width, float height);

	/// Adjust sprite display size and cutout size to match the bound texture's original dimensions
	void AdjustTextureSize();

	/// 2D Transform & Sprite properties
	const Vector2& GetPosition() const { return position_; }
	void SetPosition(const Vector2& position);

	float GetRotation() const { return rotation_; }
	void SetRotation(float rotation);

	const Vector2& GetSize() const { return size_; }
	void SetSize(const Vector2& size);

	const Vector4& GetColor() const;
	void SetColor(const Vector4& color);

	const Vector2& GetAnchorPoint() const { return anchorPoint_; }
	void SetAnchorPoint(const Vector2& anchorPoint);

	bool GetIsFlipX() const { return isFlipX_; }
	void SetIsFlipX(bool isFlipX);

	bool GetIsFlipY() const { return isFlipY_; }
	void SetIsFlipY(bool isFlipY);

	/// Dynamically recalculate and upload vertex positions and normalized UV coordinates to the vertex buffer
	void UpdateVertexBuffer();

public:
	Transform transformSprite{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

private:
	DirectXCommon* dxCommon_ = nullptr;
	// Per-frame data is kept on the CPU and uploaded to the per-frame upload allocator in Draw()
	Material materialSprite_{};
	DirectionalLightData directionalLightSprite_{};
	TransformationMatrix transformationMatrixSprite_{};
	VertexData vertexDataSprite_[6] = {};
	bool initialized_ = false;
	ID3D12DescriptorHeap* srvDescriptorHeap = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPUSprite{};
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite = nullptr;
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};

	uint32_t textureHandle_ = 0;
	Vector2 position_ = { 0.0f, 0.0f };
	float rotation_ = 0.0f;
	Vector2 size_ = { 640.0f, 360.0f };
	Vector2 anchorPoint_ = { 0.0f, 0.0f };
	bool isFlipX_ = false;
	bool isFlipY_ = false;

	// Texture cutout range in pixels (top-left coordinate and width/height)
	Vector2 textureLeftTop_ = { 0.0f, 0.0f };
	Vector2 textureSize_ = { 100.0f, 100.0f };
	bool hasCustomTextureRect_ = false;
};
