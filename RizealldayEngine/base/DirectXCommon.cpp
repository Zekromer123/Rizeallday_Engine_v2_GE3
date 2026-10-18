#include "base/DirectXCommon.h"
#include "base/Log.h"
#include "base/ConvertString.h"
#include "base/WinApp.h"
#include <dxgidebug.h>
#include <format>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib,"dxguid.lib")

DirectXCommon* DirectXCommon::GetInstance()
{
	static DirectXCommon instance;
	return &instance;
}

void DirectXCommon::Initialize(HWND hwnd, uint32_t width, uint32_t height)
{
	hwnd_ = hwnd;
	kClientWidth = width;
	kClientHeight = height;
	isDeviceLost_ = false;
	frameIndex_ = 0;

	// Enable D3D12 Debug Layer in Debug and Development builds
#if defined(_DEBUG) || defined(USE_IMGUI)
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
#ifdef _DEBUG
		debugController->SetEnableGPUBasedValidation(TRUE);
#endif
	}
#endif
	DirectXCommon::CreateFactory();
	DirectXCommon::CreateAdapter();
	DirectXCommon::CreateDevice();
	DirectXCommon::CreateCommand();
	DirectXCommon::CreateSwapChain(hwnd);
	DirectXCommon::CreateFence();
	DirectXCommon::DescriptorHeap();

	srvDescriptorHeap.Attach(CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSrvDescriptors, true));
	depthStencilResource.Attach(CreateDepthStencilTextureResource(device.Get(), kClientWidth, kClientHeight));
	dsvDescriptorHeap.Attach(CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, kMaxDsvDescriptors, false));
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	//ImGui
#ifdef USE_IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(
		device.Get(),
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap.Get(),
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
	);
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Build();

#endif // USE_IMGUI

	FlushDebugMessages();
}

void DirectXCommon::ResizeBuffers(uint32_t width, uint32_t height)
{
	if (isDeviceLost_ || width == 0 || height == 0 || (width == kClientWidth && height == kClientHeight)) {
		return;
	}
	if (!device || !swapChain) return;

	// Wait for GPU to finish rendering before resizing buffers
	SignalAndWaitFence();
	ProcessDeferredReleases(true);

	// Release references to current back buffers and depth stencil
	for (uint32_t i = 0; i < 2; ++i) {
		swapChainResources[i].Reset();
	}
	depthStencilResource.Reset();

	kClientWidth = width;
	kClientHeight = height;

	// Resize swap chain buffers
	HRESULT hr = swapChain->ResizeBuffers(
		swapChainDesc.BufferCount,
		kClientWidth,
		kClientHeight,
		swapChainDesc.Format,
		swapChainDesc.Flags
	);
	if (CheckDeviceLost(hr, "ResizeBuffers")) {
		return;
	}

	// Recreate RTVs
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	UINT rtvIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (uint32_t i = 0; i < 2; ++i) {
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainResources[i]));
		if (CheckDeviceLost(hr, "SwapChain::GetBuffer")) return;
		rtvHandles[i].ptr = rtvStartHandle.ptr + static_cast<SIZE_T>(rtvIncrement) * i;
		device->CreateRenderTargetView(swapChainResources[i].Get(), &rtvDesc, rtvHandles[i]);
	}

	// Recreate Depth Stencil
	ID3D12Resource* newDepth = CreateDepthStencilTextureResource(device.Get(), kClientWidth, kClientHeight);
	if (!newDepth) return;
	depthStencilResource.Attach(newDepth);
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Update Viewport & Scissor Rect
	viewport.Width = static_cast<float>(kClientWidth);
	viewport.Height = static_cast<float>(kClientHeight);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	scissorRect.left = 0;
	scissorRect.right = kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = kClientHeight;
}

void DirectXCommon::PreDraw()//Present -> Render Target
{
	if (isDeviceLost_ || CheckDeviceLost(S_OK, "PreDraw")) {
		return;
	}

	// Ensure buffers and viewports match actual client area every frame
	if (hwnd_) {
		RECT rc{};
		GetClientRect(hwnd_, &rc);
		int w = rc.right - rc.left;
		int h = rc.bottom - rc.top;
		if (w > 0 && h > 0) {
			if (static_cast<uint32_t>(w) != kClientWidth || static_cast<uint32_t>(h) != kClientHeight) {
				ResizeBuffers(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
				if (isDeviceLost_) return;
			}
		}
	}

	HRESULT hr;
	hr = commandAllocator->Reset();
	if (CheckDeviceLost(hr, "CommandAllocator::Reset")) return;
	hr = commandList->Reset(commandAllocator.Get(), nullptr);
	if (CheckDeviceLost(hr, "CommandList::Reset")) return;

	//RTV and DSV
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// Set Viewport and Scissor Rect to full client area
	viewport.Width = static_cast<float>(kClientWidth);
	viewport.Height = static_cast<float>(kClientHeight);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	scissorRect.left = 0;
	scissorRect.right = kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = kClientHeight;

	// Apply to Command List
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList->ResourceBarrier(1, &barrier);

	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//ImGui Frame
#ifdef USE_IMGUI
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(kClientWidth), static_cast<float>(kClientHeight));
	ImGui::NewFrame();
#endif // USE_IMGUI
}

void DirectXCommon::PostDraw()//Render Target -> Present
{
	if (isDeviceLost_) {
#ifdef USE_IMGUI
		ImGui::EndFrame();
#endif
		return;
	}

	HRESULT hr;
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

#ifdef USE_IMGUI
	//ImGui
	ImGui::Render();
	ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap.Get() };
	commandList->SetDescriptorHeaps(1, descriptorHeaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
#endif  

	//Render
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList->ResourceBarrier(1, &barrier);

	hr = commandList->Close();
	if (CheckDeviceLost(hr, "CommandList::Close")) return;

	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, commandLists);
	hr = swapChain->Present(1, 0);
	if (CheckDeviceLost(hr, "SwapChain::Present")) return;

	fenceValue++;
	hr = commandQueue->Signal(fence.Get(), fenceValue);
	if (CheckDeviceLost(hr, "CommandQueue::Signal")) return;
	if (fence->GetCompletedValue() < fenceValue)
	{
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
	CheckDeviceLost(S_OK, "PostDraw::FenceComplete");

	frameIndex_++;
	ProcessDeferredReleases(false);
	FlushDebugMessages();
}

ID3D12Resource* DirectXCommon::CreateBufferResource(size_t sizeInBytes)
{
	if (isDeviceLost_ || !device) {
		return nullptr;
	}
	HRESULT hr;
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC vertexResourceDesc{};
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeInBytes;
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ID3D12Resource* resource = nullptr;
	hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
	if (CheckDeviceLost(hr, "CreateBufferResource")) {
		return nullptr;
	}
	return resource;
}

void DirectXCommon::SignalAndWaitFence()
{
	if (isDeviceLost_ || !commandQueue || !fence) {
		return;
	}
	fenceValue++;
	HRESULT hr = commandQueue->Signal(fence.Get(), fenceValue);
	if (CheckDeviceLost(hr, "SignalAndWaitFence")) {
		return;
	}
	if (fence->GetCompletedValue() < fenceValue)
	{
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
	FlushDebugMessages();
}

void DirectXCommon::DeferRelease(Microsoft::WRL::ComPtr<IUnknown> resource)
{
	if (!resource) {
		return;
	}
	if (isDeviceLost_ || !device || !fence) {
		resource.Reset();
		return;
	}
	DeferredReleaseEntry entry{};
	entry.resource = std::move(resource);
	entry.retireFenceValue = fenceValue + 2;
	entry.retireFrameIndex = frameIndex_ + 2;
	deferredReleaseQueue_.push_back(std::move(entry));
}

void DirectXCommon::ProcessDeferredReleases(bool forceAll)
{
	if (deferredReleaseQueue_.empty()) {
		return;
	}
	if (forceAll || isDeviceLost_ || !fence) {
		deferredReleaseQueue_.clear();
		return;
	}
	uint64_t completedFence = fence->GetCompletedValue();
	if (completedFence == UINT64_MAX) {
		CheckDeviceLost(DXGI_ERROR_DEVICE_REMOVED, "ProcessDeferredReleases");
		deferredReleaseQueue_.clear();
		return;
	}
	auto it = std::remove_if(deferredReleaseQueue_.begin(), deferredReleaseQueue_.end(),
		[completedFence, this](const DeferredReleaseEntry& entry) {
			return completedFence >= entry.retireFenceValue && frameIndex_ >= entry.retireFrameIndex;
		});
	deferredReleaseQueue_.erase(it, deferredReleaseQueue_.end());
}

bool DirectXCommon::CheckDeviceLost(HRESULT hr, const char* context)
{
	if (isDeviceLost_) {
		return true;
	}
	HRESULT removedReason = S_OK;
	if (device) {
		removedReason = device->GetDeviceRemovedReason();
	}
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET ||
	    hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR || removedReason != S_OK)
	{
		isDeviceLost_ = true;
		FlushDebugMessages();
		Log::GetInstance()->logOutput(std::format(
			"[D3D12 ERROR] Device Removed/Reset detected in '{}'! hr=0x{:08X}, GetDeviceRemovedReason()=0x{:08X}. Stopping rendering.\n",
			context ? context : "Unknown",
			static_cast<uint32_t>(hr),
			static_cast<uint32_t>(removedReason)));
		return true;
	}
	if (FAILED(hr)) {
		FlushDebugMessages();
		Log::GetInstance()->logOutput(std::format(
			"[D3D12 ERROR] Operation failed in '{}' with hr=0x{:08X}\n",
			context ? context : "Unknown",
			static_cast<uint32_t>(hr)));
	}
	return false;
}

void DirectXCommon::FlushDebugMessages()
{
	if (!infoQueue) {
		return;
	}
	UINT64 numMessages = infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
	for (UINT64 i = 0; i < numMessages; ++i) {
		SIZE_T messageLength = 0;
		if (SUCCEEDED(infoQueue->GetMessage(i, nullptr, &messageLength)) && messageLength > 0) {
			std::vector<char> msgBuffer(messageLength);
			auto* msg = reinterpret_cast<D3D12_MESSAGE*>(msgBuffer.data());
			if (SUCCEEDED(infoQueue->GetMessage(i, msg, &messageLength)) && msg->pDescription) {
				Log::GetInstance()->logOutput(std::format("[D3D12 Debug] {}\n", msg->pDescription));
			}
		}
	}
	infoQueue->ClearStoredMessages();
}

void DirectXCommon::CreateFactory()
{
	//GPU
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(hr));

}

void DirectXCommon::CreateDevice()
{
	//D3D12Deviceの作成
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
	};
	const char* featureLevelString[] =
	{
		"12.2",
		"12.1",
		"12.0",
	};

	for (size_t i = 0; i < _countof(featureLevels); ++i)
	{
		HRESULT hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));
		if (SUCCEEDED(hr))
		{
			Log::GetInstance()->logOutput(std::format("Feature Level {}.\n", featureLevelString[i]));
			break;
		}
	}
	assert(device != nullptr);

	if (device && SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{
#ifdef _DEBUG
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
#endif
		D3D12_MESSAGE_ID denyIDs[] =
		{
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
		};
		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO,
		};
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIDs);
		filter.DenyList.pIDList = denyIDs;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;

		infoQueue->PushStorageFilter(&filter);
	}

	Log::GetInstance()->logOutput("Complete create D3D12Device!\n");
}

void DirectXCommon::CreateAdapter()
{
	ConvertStrings converter;

	for (UINT i = 0;
		dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND;
		++i)
	{
		DXGI_ADAPTER_DESC3 adapterDesc{};
		HRESULT hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));

		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			std::string adapterName = converter.ConvertString(adapterDesc.Description);
			Log::GetInstance()->logOutput(std::format("Using GPU: {}\n", adapterName));
			break;
		}
		useAdapter = nullptr;
	}
	assert(useAdapter != nullptr);
}

void DirectXCommon::CreateCommand()
{
	HRESULT hr;
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	assert(SUCCEEDED(hr));

	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(hr));

	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	assert(SUCCEEDED(hr));

	hr = commandList->Close();
	assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateSwapChain(HWND hwnd)
{
	swapChainDesc.Width = kClientWidth;
	swapChainDesc.Height = kClientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

void DirectXCommon::DescriptorHeap()
{
	HRESULT hr;

	// Create RTV Descriptor Heap (8 descriptors)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = kMaxRtvDescriptors;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	assert(SUCCEEDED(hr));

	// Create Render Target Views for SwapChain BackBuffers [0, 1]
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (uint32_t i = 0; i < 2; ++i)
	{
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainResources[i]));
		assert(SUCCEEDED(hr));
	}

	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	device->CreateRenderTargetView(swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);
}

void DirectXCommon::CreateFence()
{
	HRESULT hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));
	fenceValue = 0;
	fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);
}

ID3D12DescriptorHeap* DirectXCommon::CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
{
	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

ID3D12Resource* DirectXCommon::CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height)
{
	if (isDeviceLost_ || !device) {
		return nullptr;
	}
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&resource)
	);
	if (CheckDeviceLost(hr, "CreateDepthStencilTextureResource")) {
		return nullptr;
	}
	return resource;
}

void DirectXCommon::Finalize(HWND hwnd)
{
	SignalAndWaitFence();
	ProcessDeferredReleases(true);

#ifdef _DEBUG
	if (infoQueue) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
	}
#endif

#ifdef USE_IMGUI
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif // USE_IMGUI

	CloseHandle(fenceEvent);
	CloseWindow(hwnd);
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetRtvCPUDescriptorHandle(uint32_t index) const
{
	if (!device || !rtvDescriptorHeap || index >= kMaxRtvDescriptors) {
		static bool s_loggedRtvOverflow = false;
		if (!s_loggedRtvOverflow) {
			s_loggedRtvOverflow = true;
			Log::GetInstance()->logOutput(std::format("[DirectXCommon] Error: RTV descriptor index {} exceeds heap size ({})!\n", index, kMaxRtvDescriptors));
		}
		return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
	}
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)) * index;
	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetDsvCPUDescriptorHandle(uint32_t index) const
{
	if (!device || !dsvDescriptorHeap || index >= kMaxDsvDescriptors) {
		static bool s_loggedDsvOverflow = false;
		if (!s_loggedDsvOverflow) {
			s_loggedDsvOverflow = true;
			Log::GetInstance()->logOutput(std::format("[DirectXCommon] Error: DSV descriptor index {} exceeds heap size ({})!\n", index, kMaxDsvDescriptors));
		}
		return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
	}
	D3D12_CPU_DESCRIPTOR_HANDLE handle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)) * index;
	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetSrvCPUDescriptorHandle(uint32_t index) const
{
	if (!device || !srvDescriptorHeap || index >= kMaxSrvDescriptors) {
		static bool s_loggedSrvCpuOverflow = false;
		if (!s_loggedSrvCpuOverflow) {
			s_loggedSrvCpuOverflow = true;
			Log::GetInstance()->logOutput(std::format("[DirectXCommon] Error: SRV CPU descriptor index {} exceeds heap size ({})!\n", index, kMaxSrvDescriptors));
		}
		return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
	}
	D3D12_CPU_DESCRIPTOR_HANDLE handle = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(GetSrvDescriptorSize()) * index;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetSrvGPUDescriptorHandle(uint32_t index) const
{
	if (!device || !srvDescriptorHeap || index >= kMaxSrvDescriptors) {
		static bool s_loggedSrvGpuOverflow = false;
		if (!s_loggedSrvGpuOverflow) {
			s_loggedSrvGpuOverflow = true;
			Log::GetInstance()->logOutput(std::format("[DirectXCommon] Error: SRV GPU descriptor index {} exceeds heap size ({})!\n", index, kMaxSrvDescriptors));
		}
		return D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
	}
	D3D12_GPU_DESCRIPTOR_HANDLE handle = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(GetSrvDescriptorSize()) * index;
	return handle;
}

void DirectXCommon::BindMainRenderTarget()
{
	if (isDeviceLost_ || !swapChain || !commandList) {
		return;
	}
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDsvCPUDescriptorHandle(0);
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
}
