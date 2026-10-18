#include "3d/ShadowMap.h"
#include "base/DirectXCommon.h"
#include "base/DXCManager.h"
#include "base/Log.h"
#include <cassert>

void ShadowMap::Initialize(
	DirectXCommon* dxCommon,
	DXCManager* dxc,
	ID3D12RootSignature* rootSignature,
	uint32_t dsvIndex,
	uint32_t srvIndex)
{
	assert(dxCommon != nullptr);
	assert(dxc != nullptr);
	dxCommon_ = dxCommon;

	viewport_.TopLeftX = 0.0f;
	viewport_.TopLeftY = 0.0f;
	viewport_.Width = static_cast<float>(kShadowMapWidth);
	viewport_.Height = static_cast<float>(kShadowMapHeight);
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;

	scissorRect_.left = 0;
	scissorRect_.top = 0;
	scissorRect_.right = static_cast<LONG>(kShadowMapWidth);
	scissorRect_.bottom = static_cast<LONG>(kShadowMapHeight);

	CreateResource();
	CreateViews(dsvIndex, srvIndex);
	CreatePipeline(dxc, rootSignature);
}

void ShadowMap::CreateResource()
{
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = kShadowMapWidth;
	desc.Height = kShadowMapHeight;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE clearVal{};
	clearVal.Format = DXGI_FORMAT_D32_FLOAT;
	clearVal.DepthStencil.Depth = 1.0f;
	clearVal.DepthStencil.Stencil = 0;

	HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearVal,
		IID_PPV_ARGS(&textureResource_)
	);
	assert(SUCCEEDED(hr));
	currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
}

void ShadowMap::CreateViews(uint32_t dsvIndex, uint32_t srvIndex)
{
	ID3D12Device* device = dxCommon_->GetDevice();

	// DSV (D32_FLOAT)
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	dsvHandle_ = dxCommon_->GetDsvCPUDescriptorHandle(dsvIndex);
	device->CreateDepthStencilView(textureResource_.Get(), &dsvDesc, dsvHandle_);

	// SRV (R32_FLOAT)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	srvCpuHandle_ = dxCommon_->GetSrvCPUDescriptorHandle(srvIndex);
	srvGpuHandle_ = dxCommon_->GetSrvGPUDescriptorHandle(srvIndex);
	device->CreateShaderResourceView(textureResource_.Get(), &srvDesc, srvCpuHandle_);
}

void ShadowMap::CreatePipeline(DXCManager* dxc, ID3D12RootSignature* rootSignature)
{
	ID3D12Device* device = dxCommon_->GetDevice();

	if (rootSignature != nullptr) {
		rootSignature_ = rootSignature;
	} else {
		D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
		descriptorRange[0].BaseShaderRegister = 0;
		descriptorRange[0].NumDescriptors = 1;
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_DESCRIPTOR_RANGE shadowRange[1] = {};
		shadowRange[0].BaseShaderRegister = 1;
		shadowRange[0].NumDescriptors = 1;
		shadowRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		shadowRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER rootParameters[7] = {};
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[1].Descriptor.ShaderRegister = 0;

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
		rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[3].Descriptor.ShaderRegister = 1;

		rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[4].Descriptor.ShaderRegister = 1;

		rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[5].Descriptor.ShaderRegister = 2;

		rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[6].DescriptorTable.pDescriptorRanges = shadowRange;
		rootParameters[6].DescriptorTable.NumDescriptorRanges = 1;

		D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
		staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
		staticSamplers[0].ShaderRegister = 0;
		staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		staticSamplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSamplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
		staticSamplers[1].ShaderRegister = 1;
		staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC rootDesc{};
		rootDesc.pParameters = rootParameters;
		rootDesc.NumParameters = _countof(rootParameters);
		rootDesc.pStaticSamplers = staticSamplers;
		rootDesc.NumStaticSamplers = _countof(staticSamplers);
		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		Microsoft::WRL::ComPtr<ID3DBlob> sigBlob, errBlob;
		HRESULT hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
		if (FAILED(hr)) {
			if (errBlob) Log::GetInstance()->logOutput(reinterpret_cast<char*>(errBlob->GetBufferPointer()));
			assert(false);
		}
		hr = device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
		assert(SUCCEEDED(hr));
	}

	// Input Elements
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[5] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[3].SemanticName = "BLENDINDICES";
	inputElementDescs[3].SemanticIndex = 0;
	inputElementDescs[3].Format = DXGI_FORMAT_R32G32B32A32_UINT;
	inputElementDescs[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[4].SemanticName = "BLENDWEIGHT";
	inputElementDescs[4].SemanticIndex = 0;
	inputElementDescs[4].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	// Rasterizer with Depth Bias
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = 1000;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 1.5f;
	rasterizerDesc.DepthClipEnable = TRUE;

	// Depth Stencil: Depth write enabled
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	depthStencilDesc.StencilEnable = FALSE;

	// Compile Shadow.VS.hlsl
	IDxcBlob* vsBlob = dxc->CompileShader(
		L"shaders/Shadow.VS.hlsl",
		L"vs_6_0",
		dxc->GetDxcUtils(),
		dxc->GetDxcCompiler(),
		dxc->GetIncludeHandler()
	);
	assert(vsBlob != nullptr);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature_.Get();
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	psoDesc.PS = { nullptr, 0 }; // Depth-only pass, no pixel shader
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.DepthStencilState = depthStencilDesc;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.NumRenderTargets = 0; // No color render targets
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));

	vsBlob->Release();
}

void ShadowMap::BeginShadowPass(ID3D12GraphicsCommandList* cmdList)
{
	if (currentState_ != D3D12_RESOURCE_STATE_DEPTH_WRITE) {
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = textureResource_.Get();
		barrier.Transition.StateBefore = currentState_;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		cmdList->ResourceBarrier(1, &barrier);
		currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}

	cmdList->RSSetViewports(1, &viewport_);
	cmdList->RSSetScissorRects(1, &scissorRect_);
	cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle_);
	cmdList->ClearDepthStencilView(dsvHandle_, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->SetPipelineState(pipelineState_.Get());
}

void ShadowMap::EndShadowPass(ID3D12GraphicsCommandList* cmdList)
{
	if (currentState_ != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = textureResource_.Get();
		barrier.Transition.StateBefore = currentState_;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		cmdList->ResourceBarrier(1, &barrier);
		currentState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}
}
