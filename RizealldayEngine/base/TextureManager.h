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

	/// Load cubemap texture (cached)
	/// @return Texture handle ID (0 on failure)
	uint32_t LoadCubemapTexture(const std::string& filePath);

	/// Default cubemap texture handle
	uint32_t GetDefaultCubemapTexture() const { return defaultCubemapHandle_; }

	/// Load or update transient preview texture in fixed SRV slot without growing the descriptor heap
	uint32_t LoadPreviewTexture(const std::string& filePath);

	/// Release transient preview texture GPU resource when selection changes
	void ReleasePreviewTexture();

	/// Generate procedural skybox DDS cubemap
	static void CreateProceduralSkyboxDDS(const std::string& outPath);

private:
	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;

	static std::string NormalizePath(const std::string& filePath);
	uint32_t CreateDefaultCubemapTexture();
	ID3D12Resource* CreateTextureResource(const DirectX::TexMetadata& metadata);
	[[nodiscard]] ID3D12Resource* UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

	static const uint32_t kMaxTextures = 128;
	static const uint32_t kPreviewSrvBaseIndex = 133;
	static const uint32_t kPreviewSrvSlotCount = 4;
	DirectXCommon* dxCommon_ = nullptr;
	uint32_t nextIndex_ = 1;  // 0 reserved for ImGui
	uint32_t defaultWhiteHandle_ = 0;
	uint32_t defaultCubemapHandle_ = 0;
	uint32_t previewSrvRingOffset_ = 0;
	uint32_t currentPreviewSrvIndex_ = kPreviewSrvBaseIndex;
	std::unordered_map<std::string, uint32_t> textureCache_;
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResources_[kMaxTextures];
	Microsoft::WRL::ComPtr<ID3D12Resource> previewTextureResource_;
	std::string currentPreviewTexturePath_;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> uploadAllocator_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> uploadCommandList_;
};
