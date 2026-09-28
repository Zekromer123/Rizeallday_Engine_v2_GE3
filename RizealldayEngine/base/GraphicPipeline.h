#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "base/DXCManager.h"

class GraphicPipeline
{
public:
	void Initialize(ID3D12Device* pDevice, DXCManager* dxc);
	void Bind(ID3D12GraphicsCommandList* commandList);

private:
	void CreateRootSignature();
	void CreatePso(DXCManager* dxc);

	ID3D12Device* device = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
};
