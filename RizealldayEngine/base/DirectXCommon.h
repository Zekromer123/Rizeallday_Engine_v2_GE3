#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <cstdint>
#include <vector>
#include <wrl.h>

class DirectXCommon
{
public:
	// Descriptor Heap Capacities:
	// RTV Heap (8): [0..1] SwapChain, [2] Scene RT, [3] Game RT, [4] Asset Preview RT
	// DSV Heap (8): [0] Main Depth, [1] Scene Depth, [2] Game Depth, [3] ShadowMap DSV, [4] Asset Preview Depth
	// SRV Heap (256): [0] ImGui, [1..127] Cached Textures, [128] Scene SRV, [129] Game SRV, [130] ShadowMap SRV,
	//                 [131] Cubemap SRV, [132] Asset Preview RT SRV, [133..136] Preview Texture Ring, [140..249] Particles
	static constexpr uint32_t kMaxRtvDescriptors = 8;
	static constexpr uint32_t kMaxDsvDescriptors = 8;
	static constexpr uint32_t kMaxSrvDescriptors = 256;

	static DirectXCommon* GetInstance();

	void Initialize(HWND hwnd, uint32_t width = 1280, uint32_t height = 720);
	void ResizeBuffers(uint32_t width, uint32_t height);
	void PreDraw();
	void PostDraw();
	void Finalize(HWND hwnd);
	
	ID3D12Device* GetDevice() const { return device.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }
	ID3D12CommandQueue* GetCommandQueue() const { return commandQueue.Get(); }
	ID3D12DescriptorHeap* GetSrvDescriptorHeap() const { return srvDescriptorHeap.Get(); }
	ID3D12CommandAllocator* GetCommandAllocator() const { return commandAllocator.Get(); }
	uint32_t GetSrvDescriptorSize() const { return device ? device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) : 0; }
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);
	void SignalAndWaitFence();

	// Deferred GPU Resource Release Queue (freed only after the fence of the last frame that used it has completed, >= 2 frames later)
	void DeferRelease(Microsoft::WRL::ComPtr<IUnknown> resource);
	void ProcessDeferredReleases(bool forceAll = false);
	uint64_t GetCurrentFenceValue() const { return fenceValue; }
	uint64_t GetCompletedFenceValue() const { return fence ? fence->GetCompletedValue() : 0; }

	// Device Removed / Reset safety & Debug Layer message output
	bool IsDeviceLost() const { return isDeviceLost_; }
	bool CheckDeviceLost(HRESULT hr, const char* context = "");
	void FlushDebugMessages();

	uint32_t GetClientWidth() const { return kClientWidth; }
	uint32_t GetClientHeight() const { return kClientHeight; }

	// Descriptor handle helpers for offscreen RenderTexture (bounds-checked)
	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvCPUDescriptorHandle(uint32_t index) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDsvCPUDescriptorHandle(uint32_t index) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetSrvCPUDescriptorHandle(uint32_t index) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGPUDescriptorHandle(uint32_t index) const;

	// Reset render target to main BackBuffer (called after rendering Offscreen Viewports)
	void BindMainRenderTarget();

private:
	DirectXCommon() = default;
	~DirectXCommon() = default;
	DirectXCommon(const DirectXCommon&) = delete;
	DirectXCommon& operator=(const DirectXCommon&) = delete;

	void CreateFactory();
	void CreateDevice();
	void CreateAdapter();
	void CreateCommand();
	void CreateSwapChain(HWND hwnd);
	void DescriptorHeap();
	void CreateFence();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);

private:
	Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;

	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = nullptr;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

	// Descriptor Heap
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2] = {nullptr};

	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;

	// Fence & Deferred Release
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	uint64_t fenceValue = 0;
	uint64_t frameIndex_ = 0;
	HANDLE fenceEvent = nullptr;
	bool isDeviceLost_ = false;

	struct DeferredReleaseEntry {
		Microsoft::WRL::ComPtr<IUnknown> resource;
		uint64_t retireFenceValue = 0;
		uint64_t retireFrameIndex = 0;
	};
	std::vector<DeferredReleaseEntry> deferredReleaseQueue_;

	uint32_t kClientWidth = 1280;
	uint32_t kClientHeight = 720;
	HWND hwnd_ = nullptr;

	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};
};
