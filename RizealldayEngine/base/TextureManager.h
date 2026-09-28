#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <unordered_map>
#include <DirectXTex.h>

class DirectXCommon;

class TextureManager
{
public:
	static TextureManager* GetInstance();

	void Initialize(DirectXCommon* dxCommon);

	/// テクスチャ読み込み (キャッシュ付き)
	/// @return テクスチャハンドルID
	uint32_t LoadTexture(const std::string& filePath);

	/// GPU descriptor handle取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t textureHandle);

	/// SRV Descriptor Heapをコマンドリストに設定
	void SetDescriptorHeaps(ID3D12GraphicsCommandList* cmdList);

	/// デフォルト白テクスチャのハンドル
	uint32_t GetDefaultWhiteTexture() const { return defaultWhiteHandle_; }

private:
	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;

	ID3D12Resource* CreateTextureResource(const DirectX::TexMetadata& metadata);
	void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

	static const uint32_t kMaxTextures = 128;
	DirectXCommon* dxCommon_ = nullptr;
	uint32_t nextIndex_ = 1;  // 0 reserved for ImGui
	uint32_t defaultWhiteHandle_ = 0;
	std::unordered_map<std::string, uint32_t> textureCache_;
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResources_[kMaxTextures];
};
