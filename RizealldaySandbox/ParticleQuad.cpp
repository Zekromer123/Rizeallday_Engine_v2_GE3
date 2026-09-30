#include "ParticleQuad.h"
#include <cassert>

void ParticleQuad::Initialize(DirectXCommon* dxCommon, DXCManager* dxc, uint32_t textureHandle, uint32_t numInstances) {
	dxCommon_ = dxCommon;
	textureHandle_ = textureHandle;
	numInstances_ = numInstances;
	auto* device = dxCommon_->GetDevice();

	// 1. Data 6 titik Quad (板ポリ) Sesuai Slide 1
	VertexData quadVertices[6] = {
		// Segitiga 1 (Kiri Atas, Kanan Atas, Kiri Bawah)
		{ {  1.0f,  1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
		{ { -1.0f,  1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
		{ {  1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
		// Segitiga 2 (Kiri Bawah, Kanan Atas, Kanan Bawah)
		{ {  1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
		{ { -1.0f,  1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
		{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
	};

	D3D12_HEAP_PROPERTIES heapUpload{};
	heapUpload.Type = D3D12_HEAP_TYPE_UPLOAD;

	// Buat Vertex Buffer
	size_t vbSize = sizeof(quadVertices);
	D3D12_RESOURCE_DESC vbDesc{};
	vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vbDesc.Width = vbSize;
	vbDesc.Height = 1; vbDesc.DepthOrArraySize = 1; vbDesc.MipLevels = 1;
	vbDesc.SampleDesc.Count = 1;
	vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	device->CreateCommittedResource(&heapUpload, D3D12_HEAP_FLAG_NONE, &vbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer_));

	void* mappedVB = nullptr;
	vertexBuffer_->Map(0, nullptr, &mappedVB);
	memcpy(mappedVB, quadVertices, vbSize);
	vertexBuffer_->Unmap(0, nullptr);

	vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vbView_.SizeInBytes = static_cast<UINT>(vbSize);
	vbView_.StrideInBytes = sizeof(VertexData);

	// 2. Buat Material Buffer (Color, Lighting)
	D3D12_RESOURCE_DESC matDesc = vbDesc;
	matDesc.Width = (sizeof(Material) + 255) & ~255;
	device->CreateCommittedResource(&heapUpload, D3D12_HEAP_FLAG_NONE, &matDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&materialResource_));

	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	materialData_->color = color;
	materialData_->enableLighting = 0;
	materialData_->lightingType = 0;

	// 3. Buat StructuredBuffer untuk Instancing (Sesuai Slide 5)
	D3D12_RESOURCE_DESC instDesc = vbDesc;
	instDesc.Width = sizeof(TransformationMatrix) * numInstances_;
	device->CreateCommittedResource(&heapUpload, D3D12_HEAP_FLAG_NONE, &instDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&instancingResource_));

	instancingResource_->Map(0, nullptr, reinterpret_cast<void**>(&instancingData_));

	// 4. Buat SRV di Descriptor Heap (Sesuai Slide 5)
	uint32_t srvIndex = 20; // Slot 20 aman dari tekstur
	uint32_t descSize = dxCommon_->GetSrvDescriptorSize();

	D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvCPU = dxCommon_->GetSrvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	instancingSrvCPU.ptr += descSize * srvIndex;

	instancingSrvGPU_ = dxCommon_->GetSrvDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	instancingSrvGPU_.ptr += descSize * srvIndex;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Buffer.NumElements = numInstances_;
	srvDesc.Buffer.StructureByteStride = sizeof(TransformationMatrix);

	device->CreateShaderResourceView(instancingResource_.Get(), &srvDesc, instancingSrvCPU);

	// 5. Inisialisasi Transforms (Sesuai Slide 6)
	transforms_.resize(numInstances_);
	for (uint32_t i = 0; i < numInstances_; ++i) {
		transforms_[i].scale = { 0.7f, 0.7f, 0.7f };
		transforms_[i].rotate = { 0.0f, 0.0f, 0.0f };
		// Posisi berjejer diagonal bertingkat ke belakang (seperti Slide Congratulations)
		transforms_[i].translate = { i * 0.25f, i * 0.15f, i * 0.8f };
	}

	// 6. Buat Pipeline (RootSignature + PSO)
	CreatePipeline(dxc);
}

void ParticleQuad::CreatePipeline(DXCManager* dxc) {
	auto* device = dxCommon_->GetDevice();

	// Root Signature
	D3D12_DESCRIPTOR_RANGE srvRanges[2] = {};
	// Range 0: StructuredBuffer (t0, Vertex)
	srvRanges[0].BaseShaderRegister = 0;
	srvRanges[0].NumDescriptors = 1;
	srvRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// Range 1: Texture (t0, Pixel)
	srvRanges[1].BaseShaderRegister = 0;
	srvRanges[1].NumDescriptors = 1;
	srvRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParams[3] = {};
	// [0] Material CBV (b0, Pixel)
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParams[0].Descriptor.ShaderRegister = 0;

	// [1] StructuredBuffer Table (t0, Vertex) - Sesuai Slide 3 & 7!
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParams[1].DescriptorTable.pDescriptorRanges = &srvRanges[0];
	rootParams[1].DescriptorTable.NumDescriptorRanges = 1;

	// [2] Texture Table (t0, Pixel)
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParams[2].DescriptorTable.pDescriptorRanges = &srvRanges[1];
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;

	// Static Sampler
	D3D12_STATIC_SAMPLER_DESC sampler{};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rsDesc{};
	rsDesc.pParameters = rootParams;
	rsDesc.NumParameters = _countof(rootParams);
	rsDesc.pStaticSamplers = &sampler;
	rsDesc.NumStaticSamplers = 1;
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Microsoft::WRL::ComPtr<ID3DBlob> sigBlob, errBlob;
	HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
	assert(SUCCEEDED(hr));
	hr = device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));

	// Input Layout
	D3D12_INPUT_ELEMENT_DESC inputDescs[5] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// Compile Shaders
	IDxcBlob* vsBlob = dxc->CompileShader(L"shaders/Particle.VS.hlsl", L"vs_6_0", dxc->GetDxcUtils(), dxc->GetDxcCompiler(), dxc->GetIncludeHandler());
	assert(vsBlob != nullptr);
	IDxcBlob* psBlob = dxc->CompileShader(L"shaders/Particle.PS.hlsl", L"ps_6_0", dxc->GetDxcUtils(), dxc->GetDxcCompiler(), dxc->GetIncludeHandler());
	assert(psBlob != nullptr);

	// Blend Desc (Alpha Blending)
	D3D12_BLEND_DESC blendDesc{};
	D3D12_RENDER_TARGET_BLEND_DESC& rt = blendDesc.RenderTarget[0];
	rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	rt.BlendEnable = TRUE;
	rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rt.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	rt.BlendOp = D3D12_BLEND_OP_ADD;
	rt.SrcBlendAlpha = D3D12_BLEND_ONE;
	rt.DestBlendAlpha = D3D12_BLEND_ZERO;
	rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;

	// Depth Stencil (Enable depth test & write)
	D3D12_DEPTH_STENCIL_DESC depthDesc{};
	depthDesc.DepthEnable = true;
	depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

	// Rasterizer (Cull None agar partikel tampak bolak-balik)
	D3D12_RASTERIZER_DESC rastDesc{};
	rastDesc.CullMode = D3D12_CULL_MODE_NONE;
	rastDesc.FillMode = D3D12_FILL_MODE_SOLID;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature_.Get();
	psoDesc.InputLayout = { inputDescs, _countof(inputDescs) };
	psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	psoDesc.BlendState = blendDesc;
	psoDesc.RasterizerState = rastDesc;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	psoDesc.DepthStencilState = depthDesc;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));

	vsBlob->Release();
	psBlob->Release();
}

void ParticleQuad::Update(const Matrix4x4& viewProjectionMatrix) {
	if (materialData_) {
		materialData_->color = color;
	}

	// Update matriks untuk 10 quad (Sesuai Slide 6)
	for (uint32_t i = 0; i < numInstances_; ++i) {
		Matrix4x4 world = Mathematics::MakeAffineMatrix(transforms_[i].scale, transforms_[i].rotate, transforms_[i].translate);
		instancingData_[i].WVP = Mathematics::Multiply(world, viewProjectionMatrix);
		instancingData_[i].World = world;
	}
}

void ParticleQuad::Draw(ID3D12GraphicsCommandList* cmdList) {
	if (!pipelineState_) return;

	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->SetPipelineState(pipelineState_.Get());

	// [0] Material CBV
	cmdList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

	// [1] StructuredBuffer Descriptor Table (Instancing) - Sesuai Slide 3 & 7!
	cmdList->SetGraphicsRootDescriptorTable(1, instancingSrvGPU_);

	// [2] Texture Descriptor Table
	cmdList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));

	// Draw Call! Sesuai Slide 7
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView_);
	cmdList->DrawInstanced(6, numInstances_, 0, 0);
}
