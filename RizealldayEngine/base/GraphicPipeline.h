#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "base/DXCManager.h"
#include "base/BlendMode.h"

class GraphicPipeline
{
public:
    void Initialize(ID3D12Device* pDevice, DXCManager* dxc);
    void Bind(ID3D12GraphicsCommandList* commandList, BlendMode mode = BlendMode::Opaque);

private:
    void CreateRootSignature();
    void CreatePso(DXCManager* dxc, BlendMode mode);
    D3D12_BLEND_DESC MakeBlendDesc(BlendMode mode);

    ID3D12Device* device = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState[static_cast<int>(BlendMode::Count)];
};
