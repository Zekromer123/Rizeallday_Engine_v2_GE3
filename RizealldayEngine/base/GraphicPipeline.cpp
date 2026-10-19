#include "base/GraphicPipeline.h"
#include "base/DXCManager.h"
#include "base/Log.h"
#include <algorithm>

void GraphicPipeline::Initialize(ID3D12Device* pDevice, DXCManager* dxc)
{
	this->device = pDevice;
	CreateRootSignature();
	if (!rootSignature) {
		return;
	}
	// Create PSOs for all blend modes
	for (int i = 0; i < static_cast<int>(BlendMode::Count); i++) {
		CreatePso(dxc, static_cast<BlendMode>(i));
	}
}

void GraphicPipeline::Bind(ID3D12GraphicsCommandList* commandList, BlendMode mode, GeometryShaderMode gsMode)
{
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	int idx = std::clamp(static_cast<int>(mode), 0, static_cast<int>(BlendMode::Count) - 1);
	if (gsMode == GeometryShaderMode::PassThrough && graphicsPipelineStateGS[idx]) {
		commandList->SetPipelineState(graphicsPipelineStateGS[idx].Get());
	} else if (gsMode == GeometryShaderMode::Explode && graphicsPipelineStateGS_Explode[idx]) {
		commandList->SetPipelineState(graphicsPipelineStateGS_Explode[idx].Get());
	} else {
		commandList->SetPipelineState(graphicsPipelineState[idx].Get());
	}
}

void GraphicPipeline::Bind(ID3D12GraphicsCommandList* commandList, BlendMode mode, bool useGeometryShader)
{
	Bind(commandList, mode, useGeometryShader ? GeometryShaderMode::PassThrough : GeometryShaderMode::None);
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
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	Log* logger = Log::GetInstance();
	HRESULT hr;
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE shadowRange[1] = {};
	shadowRange[0].BaseShaderRegister = 1; // t1
	shadowRange[0].NumDescriptors = 1;
	shadowRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	shadowRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// Root Parameter Layout (index -> type -> register -> visibility):
	// [0] -> CBV              -> register(b3) -> D3D12_SHADER_VISIBILITY_ALL    (Material: Object3d.PS.hlsl & Object3d.Explode.GS.hlsl)
	// [1] -> CBV              -> register(b0) -> D3D12_SHADER_VISIBILITY_VERTEX (TransformationMatrix: Object3d.VS.hlsl)
	// [2] -> DESCRIPTOR_TABLE -> register(t0) -> D3D12_SHADER_VISIBILITY_PIXEL  (Texture SRV: Object3d.PS.hlsl)
	// [3] -> CBV              -> register(b1) -> D3D12_SHADER_VISIBILITY_PIXEL  (LightGroup: Object3d.PS.hlsl)
	// [4] -> CBV              -> register(b1) -> D3D12_SHADER_VISIBILITY_VERTEX (BonePalette: Object3d.VS.hlsl)
	// [5] -> CBV              -> register(b2) -> D3D12_SHADER_VISIBILITY_PIXEL  (CameraForGPU: Object3d.PS.hlsl)
	// [6] -> DESCRIPTOR_TABLE -> register(t1) -> D3D12_SHADER_VISIBILITY_PIXEL  (ShadowMap SRV: Object3d.PS.hlsl)
	D3D12_ROOT_PARAMETER rootParamaters[7] = {};
	rootParamaters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamaters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParamaters[0].Descriptor.ShaderRegister = 3;
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
	rootParamaters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamaters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParamaters[5].Descriptor.ShaderRegister = 2;
	rootParamaters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParamaters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParamaters[6].DescriptorTable.pDescriptorRanges = shadowRange;
	rootParamaters[6].DescriptorTable.NumDescriptorRanges = _countof(shadowRange);

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
	staticSamplers[1].ShaderRegister = 1; // s1
	staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);
	descriptionRootSignature.pParameters = rootParamaters;
	descriptionRootSignature.NumParameters = _countof(rootParamaters);
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr) || !signatureBlob)
	{
		if (errorBlob) {
			logger->logOutput(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		}
		assert(false);
		return;
	}
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));
}

void GraphicPipeline::CreatePso(DXCManager* dxc, BlendMode mode)
{
	HRESULT hr;

	// DEPTH STENCIL STATE
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	// Transparent objects (AlphaBlend / Add) do not write to the depth buffer
	// so they do not occlude objects behind them
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
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxc->CompileShader(L"shaders/Object3d.VS.hlsl", L"vs_6_0", dxc->GetDxcUtils(), dxc->GetDxcCompiler(), dxc->GetIncludeHandler());
	assert(vertexShaderBlob != nullptr);
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxc->CompileShader(L"shaders/Object3d.PS.hlsl", L"ps_6_0", dxc->GetDxcUtils(), dxc->GetDxcCompiler(), dxc->GetIncludeHandler());
	assert(pixelShaderBlob != nullptr);
	Microsoft::WRL::ComPtr<IDxcBlob> geometryShaderBlob = dxc->CompileShader(L"shaders/Object3d.GS.hlsl", L"gs_6_0", dxc->GetDxcUtils(), dxc->GetDxcCompiler(), dxc->GetIncludeHandler());

	// Build Standard PSO (No GS)
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

	// Build Geometry Shader PSO (Pass-Through)
	if (geometryShaderBlob) {
		D3D12_GRAPHICS_PIPELINE_STATE_DESC gsDesc = graphicsPipelineStateDesc;
		gsDesc.GS = { geometryShaderBlob->GetBufferPointer(), geometryShaderBlob->GetBufferSize() };
		hr = device->CreateGraphicsPipelineState(&gsDesc, IID_PPV_ARGS(&graphicsPipelineStateGS[static_cast<int>(mode)]));
		assert(SUCCEEDED(hr));
	}

	// Build Geometry Shader PSO (Explode Faces Along Normals)
	Microsoft::WRL::ComPtr<IDxcBlob> explodeShaderBlob = dxc->CompileShader(L"shaders/Object3d.Explode.GS.hlsl", L"gs_6_0", dxc->GetDxcUtils(), dxc->GetDxcCompiler(), dxc->GetIncludeHandler());
	if (explodeShaderBlob) {
		D3D12_GRAPHICS_PIPELINE_STATE_DESC gsExplodeDesc = graphicsPipelineStateDesc;
		gsExplodeDesc.GS = { explodeShaderBlob->GetBufferPointer(), explodeShaderBlob->GetBufferSize() };
		gsExplodeDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // Allow seeing both sides of exploded faces
		hr = device->CreateGraphicsPipelineState(&gsExplodeDesc, IID_PPV_ARGS(&graphicsPipelineStateGS_Explode[static_cast<int>(mode)]));
		assert(SUCCEEDED(hr));
	}
}
