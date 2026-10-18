#include "ParticleQuad.h"
#include <cassert>
#include <algorithm>
#include <cmath>

// Static shared pipeline resources
static Microsoft::WRL::ComPtr<ID3D12RootSignature> s_rootSignature;
static Microsoft::WRL::ComPtr<ID3D12PipelineState> s_psoNormal;
static Microsoft::WRL::ComPtr<ID3D12PipelineState> s_psoAdd;
static Microsoft::WRL::ComPtr<ID3D12Resource> s_vertexBuffer;
static D3D12_VERTEX_BUFFER_VIEW s_vbView{};

// Static descriptor allocator for particle instancing SRVs
static uint32_t s_nextParticleSrvIndex = 140;
static std::vector<uint32_t> s_freeParticleSrvIndices;

struct RetiredParticleSrv {
	uint32_t srvIndex;
	uint64_t retireFenceValue;
};
static std::vector<RetiredParticleSrv> s_retiredParticleSrvs;

static void ProcessRetiredParticleSrvs(DirectXCommon* dxCommon) {
	if (!dxCommon) return;
	uint64_t completedFence = dxCommon->GetCompletedFenceValue();
	for (auto it = s_retiredParticleSrvs.begin(); it != s_retiredParticleSrvs.end(); ) {
		if (completedFence >= it->retireFenceValue) {
			s_freeParticleSrvIndices.push_back(it->srvIndex);
			it = s_retiredParticleSrvs.erase(it);
		} else {
			++it;
		}
	}
}

static uint32_t AllocateParticleSrvIndex(DirectXCommon* dxCommon) {
	ProcessRetiredParticleSrvs(dxCommon);
	if (!s_freeParticleSrvIndices.empty()) {
		uint32_t idx = s_freeParticleSrvIndices.back();
		s_freeParticleSrvIndices.pop_back();
		return idx;
	}
	if (s_nextParticleSrvIndex < 250) {
		return s_nextParticleSrvIndex++;
	}
	return 140;
}

static void FreeParticleSrvIndex(uint32_t idx, DirectXCommon* dxCommon) {
	if (idx >= 140 && idx < 250) {
		if (dxCommon) {
			uint64_t retireFence = dxCommon->GetCurrentFenceValue() + 2;
			s_retiredParticleSrvs.push_back({ idx, retireFence });
		} else {
			s_freeParticleSrvIndices.push_back(idx);
		}
	}
}

ParticleQuad::~ParticleQuad() {
	for (uint32_t i = 0; i < kMaxPasses; ++i) {
		if (instancingResource_[i]) {
			instancingResource_[i]->Unmap(0, nullptr);
			if (dxCommon_) {
				dxCommon_->DeferRelease(instancingResource_[i]);
			}
			instancingResource_[i].Reset();
		}
		if (srvIndex_[i] != 0) {
			FreeParticleSrvIndex(srvIndex_[i], dxCommon_);
			srvIndex_[i] = 0;
		}
	}
	if (cameraResource_) {
		cameraResource_->Unmap(0, nullptr);
		if (dxCommon_) {
			dxCommon_->DeferRelease(cameraResource_);
		}
		cameraResource_.Reset();
	}
	if (materialResource_) {
		materialResource_->Unmap(0, nullptr);
		if (dxCommon_) {
			dxCommon_->DeferRelease(materialResource_);
		}
		materialResource_.Reset();
	}
}

ParticleQuad::ParticleQuad(ParticleQuad&& other) noexcept {
	*this = std::move(other);
}

ParticleQuad& ParticleQuad::operator=(ParticleQuad&& other) noexcept {
	if (this != &other) {
		for (uint32_t i = 0; i < kMaxPasses; ++i) {
			if (srvIndex_[i] != 0) {
				FreeParticleSrvIndex(srvIndex_[i], dxCommon_);
				srvIndex_[i] = 0;
			}
			if (instancingResource_[i]) {
				instancingResource_[i]->Unmap(0, nullptr);
				if (dxCommon_) {
					dxCommon_->DeferRelease(instancingResource_[i]);
				}
				instancingResource_[i].Reset();
			}
		}
		if (cameraResource_) {
			cameraResource_->Unmap(0, nullptr);
			if (dxCommon_) {
				dxCommon_->DeferRelease(cameraResource_);
			}
			cameraResource_.Reset();
		}
		if (materialResource_) {
			materialResource_->Unmap(0, nullptr);
			if (dxCommon_) {
				dxCommon_->DeferRelease(materialResource_);
			}
			materialResource_.Reset();
		}

		dxCommon_ = other.dxCommon_;
		maxParticles_ = other.maxParticles_;
		textureHandle_ = other.textureHandle_;
		for (uint32_t i = 0; i < kMaxPasses; ++i) {
			srvIndex_[i] = other.srvIndex_[i];
			instancingResource_[i] = std::move(other.instancingResource_[i]);
			instancingData_[i] = other.instancingData_[i];
			instancingSrvGPU_[i] = other.instancingSrvGPU_[i];
		}
		materialResource_ = std::move(other.materialResource_);
		materialData_ = other.materialData_;
		cameraResource_ = std::move(other.cameraResource_);
		cameraMappedData_ = other.cameraMappedData_;
		particles_ = std::move(other.particles_);
		randomEngine_ = std::move(other.randomEngine_);
		useBillboard = other.useBillboard;
		isUpdate = other.isUpdate;
		blendType = other.blendType;
		emitter = other.emitter;
		windField = other.windField;
		globalColor = other.globalColor;

		other.dxCommon_ = nullptr;
		for (uint32_t i = 0; i < kMaxPasses; ++i) {
			other.srvIndex_[i] = 0;
			other.instancingData_[i] = nullptr;
			other.instancingSrvGPU_[i] = {};
		}
		other.materialData_ = nullptr;
		other.cameraMappedData_ = nullptr;
	}
	return *this;
}

void ParticleQuad::Initialize(DirectXCommon* dxCommon, DXCManager* dxc, uint32_t textureHandle, uint32_t maxParticles) {
	dxCommon_ = dxCommon;
	textureHandle_ = textureHandle;
	maxParticles_ = (std::clamp)(maxParticles, 1u, kMaxCapacity);
	auto* device = dxCommon_->GetDevice();

	D3D12_HEAP_PROPERTIES heapUpload{};
	heapUpload.Type = D3D12_HEAP_TYPE_UPLOAD;

	// 1. Create or Share Pipeline & Shared Vertex Buffer
	CreatePipeline(dxc);

	// 2. Create Material Buffer
	D3D12_RESOURCE_DESC matDesc{};
	matDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	matDesc.Width = (sizeof(Material) + 255) & ~255;
	matDesc.Height = 1; matDesc.DepthOrArraySize = 1; matDesc.MipLevels = 1;
	matDesc.SampleDesc.Count = 1;
	matDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	device->CreateCommittedResource(&heapUpload, D3D12_HEAP_FLAG_NONE, &matDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&materialResource_));

	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	if (materialData_) {
		materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		materialData_->enableLighting = 0;
		materialData_->lightingType = 0;
	}

	// 3. Create Camera Constant Buffer (2 x 256 bytes for Scene and Game passes)
	D3D12_RESOURCE_DESC camDesc = matDesc;
	camDesc.Width = 256 * kMaxPasses;
	device->CreateCommittedResource(&heapUpload, D3D12_HEAP_FLAG_NONE, &camDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cameraResource_));
	cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraMappedData_));

	// 4. Create StructuredBuffers for Instancing (one per pass: Scene and Game)
	uint32_t descSize = dxCommon_->GetSrvDescriptorSize();
	D3D12_CPU_DESCRIPTOR_HANDLE heapStartCPU = dxCommon_->GetSrvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE heapStartGPU = dxCommon_->GetSrvDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();

	for (uint32_t pass = 0; pass < kMaxPasses; ++pass) {
		D3D12_RESOURCE_DESC instDesc = matDesc;
		instDesc.Width = sizeof(ParticleForGPU) * kMaxCapacity;
		device->CreateCommittedResource(&heapUpload, D3D12_HEAP_FLAG_NONE, &instDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&instancingResource_[pass]));

		instancingResource_[pass]->Map(0, nullptr, reinterpret_cast<void**>(&instancingData_[pass]));

		if (srvIndex_[pass] == 0) {
			srvIndex_[pass] = AllocateParticleSrvIndex(dxCommon_);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvCPU = heapStartCPU;
		instancingSrvCPU.ptr += static_cast<SIZE_T>(descSize) * srvIndex_[pass];

		instancingSrvGPU_[pass] = heapStartGPU;
		instancingSrvGPU_[pass].ptr += static_cast<SIZE_T>(descSize) * srvIndex_[pass];

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc.Buffer.NumElements = kMaxCapacity;
		srvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);

		device->CreateShaderResourceView(instancingResource_[pass].Get(), &srvDesc, instancingSrvCPU);
	}
}

void ParticleQuad::CreatePipeline(DXCManager* dxc) {
	if (s_rootSignature) return; // Already initialized once for the process

	auto* device = dxCommon_->GetDevice();

	// 1. Shared Quad Vertices
	VertexData quadVertices[6] = {
		{ {  0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
		{ { -0.5f,  0.5f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
		{ {  0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } },
		{ {  0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } },
		{ { -0.5f,  0.5f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
		{ { -0.5f, -0.5f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } },
	};

	D3D12_HEAP_PROPERTIES heapUpload{};
	heapUpload.Type = D3D12_HEAP_TYPE_UPLOAD;

	size_t vbSize = sizeof(quadVertices);
	D3D12_RESOURCE_DESC vbDesc{};
	vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vbDesc.Width = vbSize;
	vbDesc.Height = 1; vbDesc.DepthOrArraySize = 1; vbDesc.MipLevels = 1;
	vbDesc.SampleDesc.Count = 1;
	vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	device->CreateCommittedResource(&heapUpload, D3D12_HEAP_FLAG_NONE, &vbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&s_vertexBuffer));

	void* mappedVB = nullptr;
	s_vertexBuffer->Map(0, nullptr, &mappedVB);
	memcpy(mappedVB, quadVertices, vbSize);
	s_vertexBuffer->Unmap(0, nullptr);

	s_vbView.BufferLocation = s_vertexBuffer->GetGPUVirtualAddress();
	s_vbView.SizeInBytes = static_cast<UINT>(vbSize);
	s_vbView.StrideInBytes = sizeof(VertexData);

	// 2. Root Signature
	D3D12_ROOT_PARAMETER rootParams[3]{};
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[0].Descriptor.ShaderRegister = 0; // b0
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_DESCRIPTOR_RANGE srvRangeInst{};
	srvRangeInst.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRangeInst.NumDescriptors = 1;
	srvRangeInst.BaseShaderRegister = 0; // t0 in VS
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[1].DescriptorTable.pDescriptorRanges = &srvRangeInst;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_DESCRIPTOR_RANGE srvRangeTex{};
	srvRangeTex.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRangeTex.NumDescriptors = 1;
	srvRangeTex.BaseShaderRegister = 0; // t0 in PS
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[2].DescriptorTable.pDescriptorRanges = &srvRangeTex;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC sampler{};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.ShaderRegister = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rsDesc{};
	rsDesc.NumParameters = 3;
	rsDesc.pParameters = rootParams;
	rsDesc.NumStaticSamplers = 1;
	rsDesc.pStaticSamplers = &sampler;
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Microsoft::WRL::ComPtr<ID3DBlob> rsBlob, errBlob;
	D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rsBlob, &errBlob);
	device->CreateRootSignature(0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(), IID_PPV_ARGS(&s_rootSignature));

	IDxcBlob* vsBlob = dxc->CompileShader(L"shaders/Particle.VS.hlsl", L"vs_6_0", dxc->GetDxcUtils(), dxc->GetDxcCompiler(), dxc->GetIncludeHandler());
	assert(vsBlob != nullptr);
	IDxcBlob* psBlob = dxc->CompileShader(L"shaders/Particle.PS.hlsl", L"ps_6_0", dxc->GetDxcUtils(), dxc->GetDxcCompiler(), dxc->GetIncludeHandler());
	assert(psBlob != nullptr);

	D3D12_INPUT_ELEMENT_DESC inputDescs[5] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = s_rootSignature.Get();
	psoDesc.InputLayout = { inputDescs, _countof(inputDescs) };
	psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.DepthStencilState.DepthEnable = true;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // Depth test ON, Depth write OFF

	// Normal Alpha PSO
	D3D12_BLEND_DESC blendAlpha{};
	auto& rtA = blendAlpha.RenderTarget[0];
	rtA.BlendEnable = TRUE;
	rtA.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rtA.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	rtA.BlendOp = D3D12_BLEND_OP_ADD;
	rtA.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtA.DestBlendAlpha = D3D12_BLEND_ZERO;
	rtA.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtA.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDesc.BlendState = blendAlpha;
	device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&s_psoNormal));

	// Additive Glow PSO
	D3D12_BLEND_DESC blendAdd{};
	auto& rtAdd = blendAdd.RenderTarget[0];
	rtAdd.BlendEnable = TRUE;
	rtAdd.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rtAdd.DestBlend = D3D12_BLEND_ONE; // Additive
	rtAdd.BlendOp = D3D12_BLEND_OP_ADD;
	rtAdd.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtAdd.DestBlendAlpha = D3D12_BLEND_ZERO;
	rtAdd.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtAdd.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDesc.BlendState = blendAdd;
	device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&s_psoAdd));

	vsBlob->Release();
	psBlob->Release();
}

void ParticleQuad::SetMaxParticles(uint32_t maxCount) {
	maxParticles_ = (std::clamp)(maxCount, 1u, kMaxCapacity);
	if (particles_.size() > maxParticles_) {
		particles_.resize(maxParticles_);
	}
}

void ParticleQuad::ClearParticles() {
	particles_.clear();
}

void ParticleQuad::ApplyPreset(int presetIndex) {
	if (presetIndex == 1) { // Smoke
		emitter.shape = 1; // Sphere
		emitter.spawnRadius = 0.3f;
		emitter.direction = { 0.0f, 1.0f, 0.0f };
		emitter.spreadAngle = 20.0f;
		emitter.speedMin = 0.5f;
		emitter.speedMax = 1.5f;
		emitter.gravity = 0.3f; // upward drift
		emitter.drag = 0.05f;
		emitter.lifetimeMin = 2.0f;
		emitter.lifetimeMax = 3.5f;
		emitter.startScale = 0.2f;
		emitter.endScale = 0.8f;
		emitter.startColor = { 0.8f, 0.8f, 0.8f, 0.6f };
		emitter.endColor = { 0.4f, 0.4f, 0.4f, 0.0f };
		blendType = BlendType::Normal;
		emitter.billboardMode = 0;
		emitter.spawnRate = 15.0f;
	} else if (presetIndex == 2) { // Water Fountain
		emitter.shape = 3; // Cone
		emitter.coneAngle = 15.0f;
		emitter.spawnRadius = 0.2f;
		emitter.direction = { 0.0f, 1.0f, 0.0f };
		emitter.spreadAngle = 15.0f;
		emitter.speedMin = 5.0f;
		emitter.speedMax = 8.0f;
		emitter.gravity = -9.8f; // downward gravity
		emitter.drag = 0.02f;
		emitter.lifetimeMin = 1.5f;
		emitter.lifetimeMax = 2.2f;
		emitter.startScale = 0.2f;
		emitter.endScale = 0.15f;
		emitter.startColor = { 0.3f, 0.8f, 1.0f, 0.85f };
		emitter.endColor = { 0.85f, 0.95f, 1.0f, 0.1f };
		blendType = BlendType::Normal;
		emitter.billboardMode = 0;
		emitter.spawnRate = 25.0f;
	} else if (presetIndex == 3) { // Fireworks
		emitter.shape = 1; // Sphere
		emitter.spawnRadius = 0.1f;
		emitter.direction = { 0.0f, 1.0f, 0.0f };
		emitter.spreadAngle = 180.0f; // 360 spherical
		emitter.speedMin = 4.0f;
		emitter.speedMax = 9.0f;
		emitter.gravity = -2.0f;
		emitter.drag = 1.2f;
		emitter.lifetimeMin = 1.5f;
		emitter.lifetimeMax = 2.5f;
		emitter.startScale = 0.25f;
		emitter.endScale = 0.05f;
		emitter.startColor = { 1.0f, 0.9f, 0.2f, 1.0f };
		emitter.endColor = { 1.0f, 0.1f, 0.5f, 0.0f };
		blendType = BlendType::Add;
		emitter.billboardMode = 2; // Velocity aligned
		emitter.spawnRate = 30.0f;
	} else if (presetIndex == 4) { // Explosion
		emitter.shape = 1; // Sphere
		emitter.spawnRadius = 0.4f;
		emitter.direction = { 0.0f, 1.0f, 0.0f };
		emitter.spreadAngle = 180.0f;
		emitter.speedMin = 6.0f;
		emitter.speedMax = 14.0f;
		emitter.gravity = 0.0f;
		emitter.drag = 2.5f;
		emitter.lifetimeMin = 0.8f;
		emitter.lifetimeMax = 1.5f;
		emitter.startScale = 0.3f;
		emitter.endScale = 1.8f;
		emitter.startColor = { 1.0f, 0.7f, 0.1f, 1.0f };
		emitter.endColor = { 0.2f, 0.2f, 0.2f, 0.0f };
		blendType = BlendType::Add;
		emitter.billboardMode = 0;
		emitter.spawnRate = 40.0f;
	} else if (presetIndex == 5) { // Torch / Campfire
		emitter.shape = 3; // Cone
		emitter.coneAngle = 20.0f;
		emitter.spawnRadius = 0.15f;
		emitter.direction = { 0.0f, 1.0f, 0.0f };
		emitter.spreadAngle = 15.0f;
		emitter.speedMin = 1.2f;
		emitter.speedMax = 2.8f;
		emitter.gravity = 0.6f;
		emitter.drag = 0.05f;
		emitter.lifetimeMin = 0.8f;
		emitter.lifetimeMax = 1.6f;
		emitter.startScale = 0.35f;
		emitter.endScale = 0.08f;
		emitter.startColor = { 1.0f, 0.95f, 0.2f, 1.0f };
		emitter.endColor = { 0.9f, 0.15f, 0.0f, 0.0f };
		blendType = BlendType::Add;
		emitter.billboardMode = 0;
		emitter.spawnRate = 20.0f;
	}

	ClearParticles();
	EmitBurst(15);
}

Vector4 ParticleQuad::GenerateRandomColor() {
	std::uniform_real_distribution<float> distColor(0.1f, 1.0f);
	return Vector4{ distColor(randomEngine_), distColor(randomEngine_), distColor(randomEngine_), 1.0f };
}

Vector3 ParticleQuad::GenerateRandomVelocity() {
	std::uniform_real_distribution<float> distXZ(-1.2f, 1.2f);
	std::uniform_real_distribution<float> distY(1.5f, 4.0f);
	return Vector3{ distXZ(randomEngine_), distY(randomEngine_), distXZ(randomEngine_) };
}

void ParticleQuad::EmitBurst(uint32_t count) {
	std::uniform_real_distribution<float> distUnit(-1.0f, 1.0f);
	std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
	std::uniform_real_distribution<float> distLife(emitter.lifetimeMin, emitter.lifetimeMax);
	std::uniform_real_distribution<float> distSpeed(emitter.speedMin, emitter.speedMax);

	Vector3 baseDir = Normalize(emitter.direction);
	if (baseDir.Length() < 0.001f) baseDir = { 0.0f, 1.0f, 0.0f };

	float spreadRad = emitter.spreadAngle * 3.14159265f / 180.0f;

	for (uint32_t i = 0; i < count; ++i) {
		if (particles_.size() >= maxParticles_) break;
		Particle p;
		p.startScale = { emitter.startScale, emitter.startScale, emitter.startScale };
		p.endScale = { emitter.endScale, emitter.endScale, emitter.endScale };
		p.scale = p.startScale;

		// 1. Spawn position based on shape (0: Point, 1: Sphere, 2: Box, 3: Cone)
		Vector3 spawnPos = emitter.translate;
		if (emitter.shape == 0) { // Point
			spawnPos = emitter.translate;
		} else if (emitter.shape == 1) { // Sphere
			Vector3 randDir = { distUnit(randomEngine_), distUnit(randomEngine_), distUnit(randomEngine_) };
			if (randDir.Length() < 0.0001f) randDir = { 0.0f, 1.0f, 0.0f };
			randDir = Normalize(randDir);
			float r = std::cbrt(dist01(randomEngine_)) * emitter.spawnRadius;
			spawnPos += randDir * r;
		} else if (emitter.shape == 2) { // Box
			spawnPos += Vector3{
				distUnit(randomEngine_) * emitter.boxExtents.x * 0.5f,
				distUnit(randomEngine_) * emitter.boxExtents.y * 0.5f,
				distUnit(randomEngine_) * emitter.boxExtents.z * 0.5f
			};
		} else if (emitter.shape == 3) { // Cone
			float coneR = std::tan(emitter.coneAngle * 3.14159265f / 180.0f) * dist01(randomEngine_) * emitter.spawnRadius;
			float angle = dist01(randomEngine_) * 6.2831853f;
			spawnPos += Vector3{ std::cos(angle) * coneR, 0.0f, std::sin(angle) * coneR };
		}
		p.translate = spawnPos;

		// 2. Velocity direction perturbed within spreadAngle
		Vector3 velDir;
		if (emitter.spreadAngle >= 179.0f) {
			Vector3 randVec = { distUnit(randomEngine_), distUnit(randomEngine_), distUnit(randomEngine_) };
			if (randVec.Length() < 0.001f) randVec = { 0.0f, 1.0f, 0.0f };
			velDir = Normalize(randVec);
		} else {
			Vector3 randVec = { distUnit(randomEngine_), distUnit(randomEngine_), distUnit(randomEngine_) };
			velDir = Normalize(baseDir + randVec * std::tan(spreadRad * 0.5f));
		}
		float speed = distSpeed(randomEngine_);
		p.velocity = velDir * speed;

		p.startColor = emitter.startColor;
		p.endColor = emitter.endColor;
		p.color = p.startColor;

		p.gravity = emitter.gravity;
		p.drag = emitter.drag;
		p.lifeTime = (std::max)(0.01f, distLife(randomEngine_));
		p.currentTime = 0.0f;

		particles_.push_back(p);
	}
}

void ParticleQuad::Simulate(float deltaTime) {
	// 1. Spawning from Emitter
	if (isUpdate && emitter.emit && deltaTime > 0.0f) {
		float period = (emitter.spawnRate > 0.0f) ? (1.0f / emitter.spawnRate) : 0.05f;
		emitter.frequencyTimer += deltaTime;
		while (emitter.frequencyTimer >= period) {
			emitter.frequencyTimer -= period;
			EmitBurst(1);
			if (particles_.size() >= maxParticles_) break;
		}
	}

	// 2. Movement & Physics Simulation
	if (isUpdate && deltaTime > 0.0f) {
		for (auto it = particles_.begin(); it != particles_.end(); ) {
			it->currentTime += deltaTime;
			if (it->currentTime >= it->lifeTime) {
				it = particles_.erase(it);
				continue;
			}

			// Gravity
			it->velocity.y += it->gravity * deltaTime;

			// Drag
			float dragFactor = std::clamp(1.0f - it->drag * deltaTime, 0.0f, 1.0f);
			it->velocity = it->velocity * dragFactor;

			// Wind acceleration field
			if (windField.enabled) {
				it->velocity += windField.acceleration * deltaTime;
			}

			// Movement integration
			it->translate += it->velocity * deltaTime;

			// Interpolate scale & color over lifetime
			float t = std::clamp(it->currentTime / it->lifeTime, 0.0f, 1.0f);
			it->scale = it->startScale + (it->endScale - it->startScale) * t;
			it->color = {
				it->startColor.x + (it->endColor.x - it->startColor.x) * t,
				it->startColor.y + (it->endColor.y - it->startColor.y) * t,
				it->startColor.z + (it->endColor.z - it->startColor.z) * t,
				it->startColor.w + (it->endColor.w - it->startColor.w) * t
			};

			++it;
		}
	}
}

void ParticleQuad::PrepareForCamera(const Camera& camera, uint32_t passIndex) {
	if (passIndex >= kMaxPasses || !instancingData_[passIndex]) return;

	if (materialData_) {
		materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	}

	Matrix4x4 cameraWorld = Mathematics::MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, camera.rotation, camera.translation);
	Matrix4x4 billboardMatrix = Mathematics::MakeIdentity4x4();

	if (useBillboard) {
		if (emitter.billboardMode == 0) { // Camera Facing
			billboardMatrix = cameraWorld;
			billboardMatrix.m[3][0] = 0.0f;
			billboardMatrix.m[3][1] = 0.0f;
			billboardMatrix.m[3][2] = 0.0f;
		} else if (emitter.billboardMode == 1) { // Y-Axis Cylindrical
			billboardMatrix = Mathematics::MakeRotateYMatrix(camera.rotation.y);
		}
	}

	if (cameraMappedData_) {
		ParticleCameraCB cb{};
		cb.viewProjection = camera.GetViewProjectionMatrix();
		cb.cameraPosition = camera.translation;
		cb.billboardRight = { cameraWorld.m[0][0], cameraWorld.m[0][1], cameraWorld.m[0][2] };
		cb.billboardUp = { cameraWorld.m[1][0], cameraWorld.m[1][1], cameraWorld.m[1][2] };
		memcpy(cameraMappedData_ + (passIndex * 256), &cb, sizeof(cb));
	}

	const Matrix4x4& vp = camera.GetViewProjectionMatrix();
	for (size_t i = 0; i < particles_.size() && i < kMaxCapacity; ++i) {
		const auto& p = particles_[i];
		Matrix4x4 scaleMatrix = Mathematics::MakeScaleMatrix(p.scale);
		Matrix4x4 translateMatrix = Mathematics::MakeTranslateMatrix(p.translate);
		Matrix4x4 world;

		if (useBillboard && emitter.billboardMode == 2) { // Velocity Aligned
			Vector3 v = p.velocity;
			float vLen = v.Length();
			if (vLen < 0.001f) {
				Matrix4x4 defaultFacing = cameraWorld;
				defaultFacing.m[3][0] = 0.0f;
				defaultFacing.m[3][1] = 0.0f;
				defaultFacing.m[3][2] = 0.0f;
				world = Mathematics::Multiply(scaleMatrix, Mathematics::Multiply(defaultFacing, translateMatrix));
			} else {
				Vector3 up = Normalize(v);
				Vector3 toCam = camera.translation - p.translate;
				if (toCam.Length() < 0.001f) toCam = { 0.0f, 0.0f, -1.0f };
				toCam = Normalize(toCam);
				Vector3 right = Cross(up, toCam);
				if (right.Length() < 0.001f) {
					right = Cross(up, Vector3{ 1.0f, 0.0f, 0.0f });
					if (right.Length() < 0.001f) right = Cross(up, Vector3{ 0.0f, 0.0f, 1.0f });
				}
				right = Normalize(right);
				Vector3 fwd = Normalize(Cross(right, up));

				Matrix4x4 rot = Mathematics::MakeIdentity4x4();
				rot.m[0][0] = right.x; rot.m[0][1] = right.y; rot.m[0][2] = right.z;
				rot.m[1][0] = up.x;    rot.m[1][1] = up.y;    rot.m[1][2] = up.z;
				rot.m[2][0] = fwd.x;   rot.m[2][1] = fwd.y;   rot.m[2][2] = fwd.z;

				world = Mathematics::Multiply(scaleMatrix, Mathematics::Multiply(rot, translateMatrix));
			}
		} else {
			world = Mathematics::Multiply(scaleMatrix, Mathematics::Multiply(billboardMatrix, translateMatrix));
		}

		instancingData_[passIndex][i].WVP = Mathematics::Multiply(world, vp);
		instancingData_[passIndex][i].World = world;

		Vector4 finalColor = p.color;
		finalColor.x *= globalColor.x;
		finalColor.y *= globalColor.y;
		finalColor.z *= globalColor.z;
		finalColor.w *= globalColor.w;
		instancingData_[passIndex][i].color = finalColor;
	}
}

void ParticleQuad::Update(float deltaTime, const Camera& camera) {
	if (deltaTime > 0.0f) {
		Simulate(deltaTime);
	}
	PrepareForCamera(camera, kPassGame);
}

void ParticleQuad::Draw(ID3D12GraphicsCommandList* cmdList, uint32_t passIndex) {
	if (passIndex >= kMaxPasses || particles_.empty() || !instancingResource_[passIndex] || !s_rootSignature) return;

	cmdList->SetGraphicsRootSignature(s_rootSignature.Get());
	cmdList->SetPipelineState((blendType == BlendType::Add) ? s_psoAdd.Get() : s_psoNormal.Get());

	// [0] Material CBV
	cmdList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

	// [1] StructuredBuffer Descriptor Table (Instancing for this pass)
	cmdList->SetGraphicsRootDescriptorTable(1, instancingSrvGPU_[passIndex]);

	// [2] Texture Descriptor Table
	cmdList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &s_vbView);
	cmdList->DrawInstanced(6, static_cast<UINT>(particles_.size()), 0, 0);
}

