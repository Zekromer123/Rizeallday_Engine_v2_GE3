#include "base/TextureManager.h"
#include "base/DirectXCommon.h"
#include "base/ConvertString.h"
#include "base/Log.h"
#include "d3dx12.h"
#include <cassert>
#include <vector>
#include <filesystem>

TextureManager* TextureManager::GetInstance()
{
	static TextureManager instance;
	return &instance;
}

void TextureManager::Initialize(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;
	nextIndex_ = 1; // 0 reserved for ImGui
	textureCache_.clear();

	// 1x1 white fallback texture
	DirectX::ScratchImage whiteImg;
	HRESULT hr = whiteImg.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 1);
	if (SUCCEEDED(hr)) {
		const DirectX::Image* img = whiteImg.GetImage(0, 0, 0);
		if (img && img->pixels) {
			uint32_t whitePixel = 0xFFFFFFFF;
			memcpy(img->pixels, &whitePixel, sizeof(whitePixel));

			uint32_t handle = nextIndex_++;
			defaultWhiteHandle_ = handle;

			ID3D12Resource* resource = CreateTextureResource(whiteImg.GetMetadata());

			dxCommon_->GetCommandAllocator()->Reset();
			dxCommon_->GetCommandList()->Reset(dxCommon_->GetCommandAllocator(), nullptr);

			ID3D12Resource* intermediateResource = UploadTextureData(resource, whiteImg);

			dxCommon_->GetCommandList()->Close();
			ID3D12CommandList* commandLists[] = { dxCommon_->GetCommandList() };
			dxCommon_->GetCommandQueue()->ExecuteCommandLists(1, commandLists);
			dxCommon_->SignalAndWaitFence();
			intermediateResource->Release();

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;

			D3D12_CPU_DESCRIPTOR_HANDLE srvCPUHandle = dxCommon_->GetSrvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
			srvCPUHandle.ptr += dxCommon_->GetSrvDescriptorSize() * handle;
			dxCommon_->GetDevice()->CreateShaderResourceView(resource, &srvDesc, srvCPUHandle);

			textureResources_[handle].Attach(resource);
		}
	}
}

uint32_t TextureManager::LoadTexture(const std::string& filePath)
{
	// Check cache
	auto it = textureCache_.find(filePath);
	if (it != textureCache_.end()) {
		return it->second;
	}

	if (!std::filesystem::exists(filePath)) {
		Log::GetInstance()->logOutput(std::format("Texture not found: {}, using default white.\n", filePath));
		return defaultWhiteHandle_;
	}

	assert(nextIndex_ < kMaxTextures && "Texture limit exceeded");

	uint32_t handle = nextIndex_;
	nextIndex_++;

	ConvertStrings convertString;
	DirectX::ScratchImage image{};
	std::wstring filePathW = convertString.ConvertString(filePath);

	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	if (FAILED(hr)) {
		// Try DDS
		hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	}
	if (FAILED(hr)) {
		Log::GetInstance()->logOutput(std::format("Failed to decode texture: {}\n", filePath));
		return defaultWhiteHandle_;
	}

	// MipMap
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

	// Create texture resource
	ID3D12Resource* resource = CreateTextureResource(metadata);

	// Upload
	dxCommon_->GetCommandAllocator()->Reset();
	dxCommon_->GetCommandList()->Reset(dxCommon_->GetCommandAllocator(), nullptr);

	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
	ID3D12Resource* intermediateResource = UploadTextureData(resource, mipImages);

	commandList->Close();
	ID3D12CommandList* commandLists[] = { commandList };
	dxCommon_->GetCommandQueue()->ExecuteCommandLists(1, commandLists);
	dxCommon_->SignalAndWaitFence();
	intermediateResource->Release();

	// Create SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	D3D12_CPU_DESCRIPTOR_HANDLE srvCPUHandle = dxCommon_->GetSrvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	srvCPUHandle.ptr += dxCommon_->GetSrvDescriptorSize() * handle;
	dxCommon_->GetDevice()->CreateShaderResourceView(resource, &srvDesc, srvCPUHandle);

	textureResources_[handle].Attach(resource);
	textureCache_[filePath] = handle;

	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetGPUHandle(uint32_t textureHandle)
{
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = dxCommon_->GetSrvDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += dxCommon_->GetSrvDescriptorSize() * textureHandle;
	return gpuHandle;
}

void TextureManager::SetDescriptorHeaps(ID3D12GraphicsCommandList* cmdList)
{
	ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
}

ID3D12Resource* TextureManager::CreateTextureResource(const DirectX::TexMetadata& metadata)
{
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);
	resourceDesc.Height = UINT(metadata.height);
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	ID3D12Resource* resource = nullptr;
	HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&resource)
	);
	assert(SUCCEEDED(hr));
	return resource;
}

ID3D12Resource* TextureManager::UploadTextureData(ID3D12Resource* textureResource, const DirectX::ScratchImage& mipImages)
{
	ID3D12Device* device = dxCommon_->GetDevice();
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(textureResource, 0, UINT(subresources.size()));
	ID3D12Resource* intermediateResource = dxCommon_->CreateBufferResource(intermediateSize);
	UpdateSubresources(commandList, textureResource, intermediateResource, 0, 0, UINT(subresources.size()), subresources.data());
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = textureResource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}
