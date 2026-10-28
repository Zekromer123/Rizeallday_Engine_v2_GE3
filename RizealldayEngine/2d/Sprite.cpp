#include "2d/Sprite.h"
#include "base/DirectXCommon.h"
#include "base/Profiler.h"
#include "base/TextureManager.h"
#include "base/WinApp.h"
#include "3d/DirectionalLight.h"
#include <cstring>

Sprite::~Sprite()
{
	Finalize();
}

void Sprite::Initialize(DirectXCommon* dxCommon, const std::string& textureFilePath)
{
	uint32_t handle = TextureManager::GetInstance()->GetDefaultWhiteTexture();
	bool loadedFromPath = false;
	if (!textureFilePath.empty()) {
		handle = TextureManager::GetInstance()->LoadTexture(textureFilePath);
		loadedFromPath = true;
	}
	Initialize(dxCommon, handle);
	if (loadedFromPath) {
		AdjustTextureSize();
	}
}

void Sprite::Initialize(DirectXCommon* dxCommon, uint32_t textureHandle)
{
	Finalize();
	dxCommon_ = dxCommon;
	this->srvDescriptorHeap = dxCommon->GetSrvDescriptorHeap();
	indexResourceSprite = dxCommon->CreateBufferResource(sizeof(uint32_t) * 6);
	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

	// Index buffer is static (written once here), so it can stay in a persistent upload resource
	uint32_t* indexDataSprite = nullptr;
	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexDataSprite[0] = 0;
	indexDataSprite[1] = 1;
	indexDataSprite[2] = 2;
	indexDataSprite[3] = 1;
	indexDataSprite[4] = 4;
	indexDataSprite[5] = 2;
	indexResourceSprite->Unmap(0, nullptr);

	// Material & Lighting
	materialSprite_ = Material{};
	materialSprite_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialSprite_.enableLighting = 0;
	materialSprite_.lightingType = 0;
	materialSprite_.receiveShadows = 0;

	directionalLightSprite_ = DirectionalLightData{};
	directionalLightSprite_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightSprite_.direction = { 0.0f, -1.0f, 0.0f };
	directionalLightSprite_.intensity = 1.0f;
	directionalLightSprite_.active = 1;

	transformationMatrixSprite_.WVP = Mathematics::MakeIdentity4x4();
	transformationMatrixSprite_.World = Mathematics::MakeIdentity4x4();
	initialized_ = true;

	// Bind initial texture and initialize cutout range to full texture dimensions
	SetTextureHandle(textureHandle);
	UpdateVertexBuffer();
}

void Sprite::Finalize()
{
	if (indexResourceSprite) {
		if (dxCommon_) dxCommon_->DeferRelease(indexResourceSprite);
		indexResourceSprite.Reset();
	}
	initialized_ = false;
}

void Sprite::SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
	textureSrvHandleGPUSprite = handle;
	uint32_t resolvedHandle = TextureManager::GetInstance()->GetTextureHandleFromGPUDescriptor(handle);
	if (resolvedHandle != 0) {
		textureHandle_ = resolvedHandle;
		if (!hasCustomTextureRect_) {
			const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureHandle_);
			textureLeftTop_ = { 0.0f, 0.0f };
			textureSize_ = {
				static_cast<float>(metadata.width),
				static_cast<float>(metadata.height)
			};
		}
	}
	UpdateVertexBuffer();
}

void Sprite::SetTextureHandle(uint32_t textureHandle)
{
	textureHandle_ = textureHandle;
	textureSrvHandleGPUSprite = TextureManager::GetInstance()->GetGPUHandle(textureHandle_);
	if (!hasCustomTextureRect_) {
		const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureHandle_);
		textureLeftTop_ = { 0.0f, 0.0f };
		textureSize_ = {
			static_cast<float>(metadata.width),
			static_cast<float>(metadata.height)
		};
	}
	UpdateVertexBuffer();
}

void Sprite::SetTextureLeftTop(const Vector2& textureLeftTop)
{
	textureLeftTop_ = textureLeftTop;
	hasCustomTextureRect_ = true;
	UpdateVertexBuffer();
}

void Sprite::SetTextureSize(const Vector2& textureSize)
{
	textureSize_ = textureSize;
	hasCustomTextureRect_ = true;
	UpdateVertexBuffer();
}

void Sprite::SetTextureRect(const Vector2& textureLeftTop, const Vector2& textureSize)
{
	textureLeftTop_ = textureLeftTop;
	textureSize_ = textureSize;
	hasCustomTextureRect_ = true;
	UpdateVertexBuffer();
}

void Sprite::SetTextureRect(float left, float top, float width, float height)
{
	SetTextureRect({ left, top }, { width, height });
}

void Sprite::AdjustTextureSize()
{
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureHandle_);
	textureSize_ = {
		static_cast<float>(metadata.width),
		static_cast<float>(metadata.height)
	};
	size_ = textureSize_;
	UpdateVertexBuffer();
}

void Sprite::SetPosition(const Vector2& position)
{
	position_ = position;
	transformSprite.translate.x = position.x;
	transformSprite.translate.y = position.y;
}

void Sprite::SetRotation(float rotation)
{
	rotation_ = rotation;
	transformSprite.rotate.z = rotation;
}

void Sprite::SetSize(const Vector2& size)
{
	size_ = size;
	UpdateVertexBuffer();
}

const Vector4& Sprite::GetColor() const
{
	static const Vector4 kDefaultWhite = { 1.0f, 1.0f, 1.0f, 1.0f };
	return initialized_ ? materialSprite_.color : kDefaultWhite;
}

void Sprite::SetColor(const Vector4& color)
{
	materialSprite_.color = color;
}

void Sprite::SetAnchorPoint(const Vector2& anchorPoint)
{
	anchorPoint_ = anchorPoint;
	UpdateVertexBuffer();
}

void Sprite::SetIsFlipX(bool isFlipX)
{
	isFlipX_ = isFlipX;
	UpdateVertexBuffer();
}

void Sprite::SetIsFlipY(bool isFlipY)
{
	isFlipY_ = isFlipY;
	UpdateVertexBuffer();
}

void Sprite::UpdateVertexBuffer()
{
	if (!initialized_) {
		return;
	}

	// Calculate vertex positions considering anchor point, flip flags, and display size
	float left = (0.0f - anchorPoint_.x) * size_.x;
	float right = (1.0f - anchorPoint_.x) * size_.x;
	float top = (0.0f - anchorPoint_.y) * size_.y;
	float bottom = (1.0f - anchorPoint_.y) * size_.y;

	if (isFlipX_) {
		left = -left;
		right = -right;
	}
	if (isFlipY_) {
		top = -top;
		bottom = -bottom;
	}

	// Calculate normalized UV coordinates (0.0 ~ 1.0) from pixel cutout rectangle and texture metadata
	float texLeft = 0.0f;
	float texRight = 1.0f;
	float texTop = 0.0f;
	float texBottom = 1.0f;

	if (textureHandle_ != 0) {
		const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureHandle_);
		float texWidth = (metadata.width > 0) ? static_cast<float>(metadata.width) : 1.0f;
		float texHeight = (metadata.height > 0) ? static_cast<float>(metadata.height) : 1.0f;

		texLeft = textureLeftTop_.x / texWidth;
		texRight = (textureLeftTop_.x + textureSize_.x) / texWidth;
		texTop = textureLeftTop_.y / texHeight;
		texBottom = (textureLeftTop_.y + textureSize_.y) / texHeight;
	}

	// First triangle
	vertexDataSprite_[0].position = { left, bottom, 0.0f, 1.0f }; // Bottom-left
	vertexDataSprite_[0].texcoord = { texLeft, texBottom };
	vertexDataSprite_[0].normal = { 0.0f, 0.0f, -1.0f };

	vertexDataSprite_[1].position = { left, top, 0.0f, 1.0f }; // Top-left
	vertexDataSprite_[1].texcoord = { texLeft, texTop };
	vertexDataSprite_[1].normal = { 0.0f, 0.0f, -1.0f };

	vertexDataSprite_[2].position = { right, bottom, 0.0f, 1.0f }; // Bottom-right
	vertexDataSprite_[2].texcoord = { texRight, texBottom };
	vertexDataSprite_[2].normal = { 0.0f, 0.0f, -1.0f };

	// Second triangle
	vertexDataSprite_[3].position = { left, top, 0.0f, 1.0f }; // Top-left
	vertexDataSprite_[3].texcoord = { texLeft, texTop };
	vertexDataSprite_[3].normal = { 0.0f, 0.0f, -1.0f };

	vertexDataSprite_[4].position = { right, top, 0.0f, 1.0f }; // Top-right
	vertexDataSprite_[4].texcoord = { texRight, texTop };
	vertexDataSprite_[4].normal = { 0.0f, 0.0f, -1.0f };

	vertexDataSprite_[5].position = { right, bottom, 0.0f, 1.0f }; // Bottom-right
	vertexDataSprite_[5].texcoord = { texRight, texBottom };
	vertexDataSprite_[5].normal = { 0.0f, 0.0f, -1.0f };
}

void Sprite::Update()
{
	position_ = { transformSprite.translate.x, transformSprite.translate.y };
	rotation_ = transformSprite.rotate.z;

	UpdateVertexBuffer();

	Matrix4x4 worldMatrixSprite = Mathematics::MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
	Matrix4x4 viewMatrixSprite = Mathematics::MakeIdentity4x4();
	Matrix4x4 projectionMatrixSprite = Mathematics::MakeOrthographicMatrix(
		0.0f, 0.0f,
		static_cast<float>(WinApp::kClientWidth),
		static_cast<float>(WinApp::kClientHeight),
		0.0f, 100.0f);
	Matrix4x4 worldViewProjectionMatrixSprite = Mathematics::Multiply(worldMatrixSprite, Mathematics::Multiply(viewMatrixSprite, projectionMatrixSprite));
	transformationMatrixSprite_.WVP = worldViewProjectionMatrixSprite;
	transformationMatrixSprite_.World = worldMatrixSprite;
}

void Sprite::Draw(ID3D12GraphicsCommandList* commandList)
{
	if (!initialized_ || !dxCommon_ || dxCommon_->IsDeviceLost()) {
		return;
	}

	// Upload this frame's vertices and constants into the per-frame upload allocator
	DirectXCommon::UploadAllocation vertexAlloc = dxCommon_->AllocateUpload(sizeof(vertexDataSprite_), 16);
	const D3D12_GPU_VIRTUAL_ADDRESS transformAddr = dxCommon_->UploadConstants(transformationMatrixSprite_);
	const D3D12_GPU_VIRTUAL_ADDRESS materialAddr = dxCommon_->UploadConstants(materialSprite_);
	const D3D12_GPU_VIRTUAL_ADDRESS lightAddr = dxCommon_->UploadConstants(directionalLightSprite_);
	if (!vertexAlloc.cpu || transformAddr == 0 || materialAddr == 0 || lightAddr == 0) {
		return;
	}
	std::memcpy(vertexAlloc.cpu, vertexDataSprite_, sizeof(vertexDataSprite_));

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexAlloc.gpu;
	vertexBufferView.SizeInBytes = sizeof(vertexDataSprite_);
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap };
	commandList->SetDescriptorHeaps(1, heaps);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->SetGraphicsRootConstantBufferView(1, transformAddr);
	commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPUSprite);
	commandList->SetGraphicsRootConstantBufferView(0, materialAddr);
	commandList->IASetIndexBuffer(&indexBufferViewSprite);

	commandList->SetGraphicsRootConstantBufferView(3, lightAddr);
	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	Profiler::GetInstance()->CountDrawCall();
}
