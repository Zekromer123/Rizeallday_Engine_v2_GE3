#include "base/GraphicPipeline.h"
#include "base/Log.h"
#include <algorithm>

void GraphicPipeline::Initialize(ID3D12Device* pDevice, DXCManager* dxc)
{
	this->device = pDevice;
	CreateRootSignature();
	// Buat PSO untuk semua blend mode
	for (int i = 0; i < static_cast<int>(BlendMode::Count); i++) {
		CreatePso(dxc, static_cast<BlendMode>(i));
	}
}

void GraphicPipeline::Bind(ID3D12GraphicsCommandList* commandList, BlendMode mode)
{
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	int idx = std::clamp(static_cast<int>(mode), 0, static_cast<int>(BlendMode::Count) - 1);
	commandList->SetPipelineState(graphicsPipelineState[idx].Get());
}

D3D12_BLEND_DESC GraphicPipeline::MakeBlendDesc(BlendMode mode)
{
	D3D12_BLEND_DESC blendDesc{};
	D3D12_RENDER_TARGET_BLEND_DESC& rt = blendDesc.RenderTarget[0];
	rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	switch (mode) {
	case BlendMode::AlphaBlend:
		rt.BlendEnable    = TRUE;
		rt.SrcBlend       = D3D12_BLEND_SRC_ALPHA;
		rt.DestBlend      = D3D12_BLEND_INV_SRC_ALPHA;
		rt.BlendOp        = D3D12_BLEND_OP_ADD;
		rt.SrcBlendAlpha  = D3D12_BLEND_ONE;
		rt.DestBlendAlpha = D3D12_BLEND_ZERO;
		rt.BlendOpAlpha   = D3D12_BLEND_OP_ADD;
		break;
	case BlendMode::Add:
		rt.BlendEnable    = TRUE;
		rt.SrcBlend       = D3D12_BLEND_SRC_ALPHA;
		rt.DestBlend      = D3D12_BLEND_ONE;
		rt.BlendOp        = D3D12_BLEND_OP_ADD;
		rt.SrcBlendAlpha  = D3D12_BLEND_ONE;
		rt.DestBlendAlpha = D3D12_BLEND_ZERO;
		rt.BlendOpAlpha   = D3D12_BLEND_OP_ADD;
		break;
	case BlendMode::Multiply:
		rt.BlendEnable    = TRUE;
		rt.SrcBlend       = D3D12_BLEND_ZERO;
		rt.DestBlend      = D3D12_BLEND_SRC_COLOR;
		rt.BlendOp        = D3D12_BLEND_OP_ADD;
		rt.SrcBlendAlpha  = D3D12_BLEND_ONE;
		rt.DestBlendAlpha = D3D12_BLEND_ZERO;
		rt.BlendOpAlpha   = D3D12_BLEND_OP_ADD;
		break;
	default: // Opaque
		rt.BlendEnable = FALSE;
		break;
	}
	return blendDesc;
}

void GraphicPipeline::CreateRootSignature()
{
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	Log* logger = Log::GetInstance();
	HRESULT hr;
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// Root Parameters:
	// [0] Material CBV (b0, Pixel) 
	// [1] TransformationMatrix CBV (b0, Vertex)
	// [2] Texture SRV Table (t0, Pixel)
	// [3] LightGroup CBV (b1, Pixel)
	// [4] BonePalette CBV (b1, Vertex)
	D3D12_ROOT_PARAMETER rootParamaters[5] = {};
	rootParamaters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamaters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParamaters[0].Descriptor.ShaderRegister = 0;
	rootParamaters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamaters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParamaters[1].Descriptor.ShaderRegister = 0;
	rootParamaters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParamaters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParamaters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParamaters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
	rootParamaters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamaters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParamaters[3].Descriptor.ShaderRegister = 1;
	rootParamaters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamaters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParamaters[4].Descriptor.ShaderRegister = 1;

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);
	descriptionRootSignature.pParameters = rootParamaters;
	descriptionRootSignature.NumParameters = _countof(rootParamaters);
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr))
	{
		logger->logOutput(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

	if (signatureBlob) signatureBlob->Release();
	if (errorBlob) errorBlob->Release();
}

void GraphicPipeline::CreatePso(DXCManager* dxc, BlendMode mode)
{
	HRESULT hr;

	// DEPTH STENCIL STATE
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	// Objek transparan (AlphaBlend / Add) tidak menulis ke depth buffer
	// agar tidak memblokir objek di belakangnya
	if (mode == BlendMode::AlphaBlend || mode == BlendMode::Add) {
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	}
	else {
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	}

	// Input Layout
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

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// Rasterizer State
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// Compile Shaders
	IDxcBlob* vertexShaderBlob = dxc->CompileShader(L"shaders/Object3d.VS.hlsl", L"vs_6_0", dxc->GetDxcUtils(), dxc->GetDxcCompiler(), dxc->GetIncludeHandler());
	assert(vertexShaderBlob != nullptr);
	IDxcBlob* pixelShaderBlob = dxc->CompileShader(L"shaders/Object3d.PS.hlsl", L"ps_6_0", dxc->GetDxcUtils(), dxc->GetDxcCompiler(), dxc->GetIncludeHandler());
	assert(pixelShaderBlob != nullptr);

	// Build PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.BlendState = MakeBlendDesc(mode);
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState[static_cast<int>(mode)]));
	assert(SUCCEEDED(hr));

	vertexShaderBlob->Release();
	pixelShaderBlob->Release();
}
