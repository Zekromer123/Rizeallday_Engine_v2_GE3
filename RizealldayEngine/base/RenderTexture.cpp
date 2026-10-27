#include "base/RenderTexture.h"
#include "base/DirectXCommon.h"
#include "base/Log.h"
#include <cassert>
#include <cstring>
#include <format>

RenderTexture::~RenderTexture()
{
	if (dxCommon_) {
		if (colorResource_) dxCommon_->DeferRelease(colorResource_);
		if (depthResource_) dxCommon_->DeferRelease(depthResource_);
	}
	colorResource_.Reset();
	depthResource_.Reset();
}

void RenderTexture::Initialize(
	DirectXCommon* dxCommon,
	uint32_t width,
	uint32_t height,
	uint32_t rtvIndex,
	uint32_t dsvIndex,
	uint32_t srvIndex,
	const float clearColor[4])
{
	dxCommon_ = dxCommon;
	width_ = width;
	height_ = height;
	isInitialized_ = false;

	if (!dxCommon_ || dxCommon_->IsDeviceLost() || !dxCommon_->GetDevice()) {
		return;
	}

	if (clearColor) {
		std::memcpy(clearColor_, clearColor, sizeof(float) * 4);
	}

	// Validate descriptor allocations before creating GPU resources
	rtvHandle_ = dxCommon_->GetRtvCPUDescriptorHandle(rtvIndex);
	dsvHandle_ = dxCommon_->GetDsvCPUDescriptorHandle(dsvIndex);
	srvCpuHandle_ = dxCommon_->GetSrvCPUDescriptorHandle(srvIndex);
	srvGpuHandle_ = dxCommon_->GetSrvGPUDescriptorHandle(srvIndex);

	if (rtvHandle_.ptr == 0 || dsvHandle_.ptr == 0 || srvCpuHandle_.ptr == 0 || srvGpuHandle_.ptr == 0) {
		static bool s_loggedDescFail = false;
		if (!s_loggedDescFail) {
			s_loggedDescFail = true;
			Log::GetInstance()->logOutput(std::format(
				"[RenderTexture] Error: Descriptor allocation failed (RTV={}, DSV={}, SRV={}). Skipping RenderTexture init.\n",
				rtvIndex, dsvIndex, srvIndex));
		}
		return;
	}

	ID3D12Device* device = dxCommon_->GetDevice();

	if (colorResource_) {
		dxCommon_->DeferRelease(colorResource_);
		colorResource_.Reset();
	}
	if (depthResource_) {
		dxCommon_->DeferRelease(depthResource_);
		depthResource_.Reset();
	}

	// 1. Create Color Render Target Texture Resource
	D3D12_RESOURCE_DESC texDesc{};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = width_;
	texDesc.Height = height_;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	// Typeless so the 3D pass can write through an sRGB RTV while ImGui reads the same bytes through a UNORM SRV
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
	texDesc.SampleDesc.Count = 1;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE colorClearVal{};
	colorClearVal.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	std::memcpy(colorClearVal.Color, clearColor_, sizeof(float) * 4);

	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&colorClearVal,
		IID_PPV_ARGS(&colorResource_)
	);
	if (dxCommon_->CheckDeviceLost(hr, "RenderTexture::ColorResource") || !colorResource_) {
		return;
	}
	currentState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	// 2. Create RTV
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	device->CreateRenderTargetView(colorResource_.Get(), &rtvDesc, rtvHandle_);

	// 3. Create SRV (for ImGui::Image). ImGui renders into a UNORM back buffer view, so the sRGB-encoded
	//    bytes are passed through unchanged (UNORM) instead of being decoded to linear (UNORM_SRGB).
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// Force alpha = 1: the viewport image must be opaque in the UI. Otherwise any pass that writes
	// alpha < 1 (particles, transparent materials, grid lines) lets the dark panel show through.
	srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
		D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
		D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
		D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
		D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(colorResource_.Get(), &srvDesc, srvCpuHandle_);

	// 4. Create Depth Stencil Resource & DSV
	D3D12_RESOURCE_DESC depthDesc{};
	depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthDesc.Width = width_;
	depthDesc.Height = height_;
	depthDesc.DepthOrArraySize = 1;
	depthDesc.MipLevels = 1;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthClearVal{};
	depthClearVal.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthClearVal.DepthStencil.Depth = 1.0f;

	hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearVal,
		IID_PPV_ARGS(&depthResource_)
	);
	if (dxCommon_->CheckDeviceLost(hr, "RenderTexture::DepthResource") || !depthResource_) {
		colorResource_.Reset();
		return;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(depthResource_.Get(), &dsvDesc, dsvHandle_);

	// 5. Setup Viewport & Scissor
	viewport_.TopLeftX = 0.0f;
	viewport_.TopLeftY = 0.0f;
	viewport_.Width = static_cast<float>(width_);
	viewport_.Height = static_cast<float>(height_);
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;

	scissorRect_.left = 0;
	scissorRect_.top = 0;
	scissorRect_.right = static_cast<LONG>(width_);
	scissorRect_.bottom = static_cast<LONG>(height_);
	isInitialized_ = true;
}

void RenderTexture::BeginRender()
{
	if (!isInitialized_ || !dxCommon_ || dxCommon_->IsDeviceLost() || !colorResource_ || !depthResource_) {
		return;
	}
	ID3D12GraphicsCommandList* cmdList = dxCommon_->GetCommandList();
	if (!cmdList) return;

	if (currentState_ != D3D12_RESOURCE_STATE_RENDER_TARGET) {
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = colorResource_.Get();
		barrier.Transition.StateBefore = currentState_;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		cmdList->ResourceBarrier(1, &barrier);
		currentState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}

	cmdList->RSSetViewports(1, &viewport_);
	cmdList->RSSetScissorRects(1, &scissorRect_);
	cmdList->OMSetRenderTargets(1, &rtvHandle_, false, &dsvHandle_);

	cmdList->ClearRenderTargetView(rtvHandle_, clearColor_, 0, nullptr);
	cmdList->ClearDepthStencilView(dsvHandle_, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void RenderTexture::EndRender()
{
	if (!isInitialized_ || !dxCommon_ || dxCommon_->IsDeviceLost() || !colorResource_) {
		return;
	}
	ID3D12GraphicsCommandList* cmdList = dxCommon_->GetCommandList();
	if (!cmdList) return;

	if (currentState_ != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = colorResource_.Get();
		barrier.Transition.StateBefore = currentState_;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		cmdList->ResourceBarrier(1, &barrier);
		currentState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}
}
