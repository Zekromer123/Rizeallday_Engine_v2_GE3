#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <cstdint>
#include <wrl.h>

#ifdef USE_IMGUI
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#endif

class DirectXCommon
{
public:
	static DirectXCommon* GetInstance();

	void Initialize(HWND hwnd, uint32_t width = 1280, uint32_t height = 720);
	void PreDraw();
	void PostDraw();
	void Finalize(HWND hwnd);
	
	ID3D12Device* GetDevice() const { return device.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }
	ID3D12CommandQueue* GetCommandQueue() const { return commandQueue.Get(); }
	ID3D12DescriptorHeap* GetSrvDescriptorHeap() const { return srvDescriptorHeap.Get(); }
	ID3D12CommandAllocator* GetCommandAllocator() const { return commandAllocator.Get(); }
	uint32_t GetSrvDescriptorSize() const { return device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); }
	ID3D12Resource* CreateBufferResource(size_t sizeInBytes);
	void SignalAndWaitFence();

	uint32_t GetClientWidth() const { return kClientWidth; }
	uint32_t GetClientHeight() const { return kClientHeight; }

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
	ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device,D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);
	ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device,int32_t width,int32_t height);

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
#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
#endif

	//Fence
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	uint64_t fenceValue = 0;
	HANDLE fenceEvent = nullptr;

	uint32_t kClientWidth = 1280;
	uint32_t kClientHeight = 720;

	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};
};
