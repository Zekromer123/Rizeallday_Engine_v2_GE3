#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "math/Matrix4x4.h"
#include "math/Vector3.h"

class DirectXCommon;
class DXCManager;

class ShadowMap
{
public:
	static const uint32_t kShadowMapWidth = 2048;
	static const uint32_t kShadowMapHeight = 2048;

	void Initialize(
		DirectXCommon* dxCommon,
		DXCManager* dxc,
		ID3D12RootSignature* rootSignature = nullptr,
		uint32_t dsvIndex = 3,
		uint32_t srvIndex = 130
	);

	void BeginShadowPass(ID3D12GraphicsCommandList* cmdList);
	void EndShadowPass(ID3D12GraphicsCommandList* cmdList);

	ID3D12PipelineState* GetPipelineState() const { return pipelineState_.Get(); }
	ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetDsvCPUHandle() const { return dsvHandle_; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGPUHandle() const { return srvGpuHandle_; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSrvCPUHandle() const { return srvCpuHandle_; }
	ID3D12Resource* GetResource() const { return textureResource_.Get(); }

private:
	void CreateResource();
	void CreateViews(uint32_t dsvIndex, uint32_t srvIndex);
	void CreatePipeline(DXCManager* dxc, ID3D12RootSignature* rootSignature);

	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_{};
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle_{};
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle_{};

	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissorRect_{};
	D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
};
