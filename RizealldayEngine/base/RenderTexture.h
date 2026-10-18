#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

class DirectXCommon;

class RenderTexture
{
public:
	~RenderTexture();

	void Initialize(
		DirectXCommon* dxCommon,
		uint32_t width,
		uint32_t height,
		uint32_t rtvIndex,
		uint32_t dsvIndex,
		uint32_t srvIndex,
		const float clearColor[4] = nullptr
	);

	void BeginRender();
	void EndRender();

	bool IsInitialized() const { return isInitialized_; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGPUHandle() const { return srvGpuHandle_; }
	uint32_t GetWidth() const { return width_; }
	uint32_t GetHeight() const { return height_; }

private:
	DirectXCommon* dxCommon_ = nullptr;
	uint32_t width_ = 1280;
	uint32_t height_ = 720;
	float clearColor_[4] = { 0.12f, 0.12f, 0.15f, 1.0f };
	bool isInitialized_ = false;
	D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	Microsoft::WRL::ComPtr<ID3D12Resource> colorResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> depthResource_ = nullptr;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_{};
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_{};
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle_{};
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle_{};

	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissorRect_{};
};
