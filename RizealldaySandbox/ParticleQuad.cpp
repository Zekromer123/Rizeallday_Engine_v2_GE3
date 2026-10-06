#include "ParticleQuad.h"
#include <cassert>
#include <algorithm>

void ParticleQuad::Initialize(DirectXCommon* dxCommon, DXCManager* dxc, uint32_t textureHandle, uint32_t maxParticles) {
	dxCommon_ = dxCommon;
	textureHandle_ = textureHandle;
	maxParticles_ = maxParticles;
	auto* device = dxCommon_->GetDevice();

	// 1. Quad Vertices
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
	materialData_->color = globalColor;
	materialData_->enableLighting = 0;
	materialData_->lightingType = 0;

	// 3. Buat StructuredBuffer untuk Instancing (Sesuai Slide 5)
	D3D12_RESOURCE_DESC instDesc = vbDesc;
	instDesc.Width = sizeof(ParticleForGPU) * maxParticles_;
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
	srvDesc.Buffer.NumElements = maxParticles_;
	srvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);

	device->CreateShaderResourceView(instancingResource_.Get(), &srvDesc, instancingSrvCPU);

	CreatePipeline(dxc);
}

void ParticleQuad::CreatePipeline(DXCManager* dxc) {
	auto* device = dxCommon_->GetDevice();

	// Root Signature
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
	device->CreateRootSignature(0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
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
	psoDesc.pRootSignature = rootSignature_.Get();
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
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // Depth write OFF untuk partikel transparan
	// --- PSO 1: kBlendModeNormal (AlphaBlend) ---
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
	device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&psoNormal_));
	// --- PSO 2: kBlendModeAdd (Additive Glow) ---
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
	device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&psoAdd_));

	vsBlob->Release();
	psBlob->Release();
}

Vector4 ParticleQuad::GenerateRandomColor() {
	std::uniform_real_distribution<float> dist(0.3f, 1.0f);
	return Vector4{ dist(randomEngine_), dist(randomEngine_), dist(randomEngine_), 1.0f };
}
Vector3 ParticleQuad::GenerateRandomVelocity() {
	std::uniform_real_distribution<float> distXZ(-1.2f, 1.2f);
	std::uniform_real_distribution<float> distY(1.5f, 4.0f);
	return Vector3{ distXZ(randomEngine_), distY(randomEngine_), distXZ(randomEngine_) };
}
void ParticleQuad::EmitBurst(uint32_t count) {
	std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
	for (uint32_t i = 0; i < count; ++i) {
		if (particles_.size() >= maxParticles_) break;
		Particle p;
		p.scale = { 0.8f, 0.8f, 0.8f };
		p.translate = emitter.translate + Vector3{
			dist(randomEngine_) * emitter.radius.x,
			dist(randomEngine_) * emitter.radius.y,
			dist(randomEngine_) * emitter.radius.z
		};
		p.velocity = GenerateRandomVelocity();
		p.color = GenerateRandomColor();
		p.lifeTime = 2.0f + ((dist(randomEngine_) + 1.0f) * 0.5f); // 2.0 - 3.0 detik
		p.currentTime = 0.0f;
		particles_.push_back(p);
	}
}

void ParticleQuad::Update(float deltaTime, const Camera& camera) {
	if (materialData_) {
		materialData_->color = globalColor;
	}

	// 1. Spawning dari Emitter
	if (isUpdate && emitter.emit) {
		emitter.frequencyTimer += deltaTime;
		if (emitter.frequencyTimer >= emitter.frequency) {
			emitter.frequencyTimer -= emitter.frequency;
			EmitBurst(emitter.count);
		}
	}
	// 2. Simulasi Pergerakan & Angin (Slide 2)
	for (auto it = particles_.begin(); it != particles_.end(); ) {
		if (isUpdate) {
			it->currentTime += deltaTime;
			if (it->currentTime >= it->lifeTime) {
				it = particles_.erase(it);
				continue;
			}
			// Pengaruh Angin (Acceleration Field - Sesuai Slide 2)
			if (windField.enabled) {
				if (it->translate.x >= windField.area.min.x && it->translate.x <= windField.area.max.x &&
					it->translate.y >= windField.area.min.y && it->translate.y <= windField.area.max.y &&
					it->translate.z >= windField.area.min.z && it->translate.z <= windField.area.max.z) {
					it->velocity = it->velocity + (windField.acceleration * deltaTime);
				}
			}
			// Gerakan
			it->translate = it->translate + (it->velocity * deltaTime);
		}
		++it;
	}
	// 3. Hitung Matriks Billboard (SESUAI RUMUS SLIDE 1)
	Matrix4x4 billboardMatrix = Mathematics::MakeIdentity4x4();
	if (useBillboard) {
		Matrix4x4 cameraWorld = Mathematics::MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, camera.rotation, camera.translation);
		billboardMatrix = cameraWorld;
		// Nolkan translasi agar hanya mengambil orientasi rotasi kamera
		billboardMatrix.m[3][0] = 0.0f;
		billboardMatrix.m[3][1] = 0.0f;
		billboardMatrix.m[3][2] = 0.0f;
	}
	// 4. Salin ke GPU StructuredBuffer (WorldMatrix = scaleMatrix * billboardMatrix * translateMatrix)
	const Matrix4x4& vp = camera.GetViewProjectionMatrix();
	for (size_t i = 0; i < particles_.size(); ++i) {
		const auto& p = particles_[i];
		Matrix4x4 scaleMatrix = Mathematics::MakeScaleMatrix(p.scale);
		Matrix4x4 translateMatrix = Mathematics::MakeTranslateMatrix(p.translate);
		// Rumus Slide 1: scaleMatrix * billboardMatrix * translateMatrix
		Matrix4x4 world = Mathematics::Multiply(scaleMatrix, Mathematics::Multiply(billboardMatrix, translateMatrix));
		instancingData_[i].WVP = Mathematics::Multiply(world, vp);
		instancingData_[i].World = world;
		// Alpha memudar seiring habisnya umur (fade-out)
		float alphaRatio = 1.0f - (p.currentTime / p.lifeTime);
		Vector4 finalColor = p.color;
		finalColor.w *= std::clamp(alphaRatio, 0.0f, 1.0f);
		instancingData_[i].color = finalColor;
	}
}

void ParticleQuad::Draw(ID3D12GraphicsCommandList* cmdList) {
	if (particles_.empty()) return;

	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->SetPipelineState((blendType == BlendType::Add) ? psoAdd_.Get() : psoNormal_.Get());
	// [0] Material CBV
	cmdList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

	// [1] StructuredBuffer Descriptor Table (Instancing) - Sesuai Slide 3 & 7!
	cmdList->SetGraphicsRootDescriptorTable(1, instancingSrvGPU_);

	// [2] Texture Descriptor Table
	cmdList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView_);
	cmdList->DrawInstanced(6, static_cast<UINT>(particles_.size()), 0, 0);
}
