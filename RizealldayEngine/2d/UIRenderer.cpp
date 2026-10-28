#include "2d/UIRenderer.h"
#include "base/DirectXCommon.h"
#include "base/DXCManager.h"
#include "base/TextureManager.h"
#include "base/Profiler.h"
#include "base/Log.h"
#include <Windows.h>
#include <algorithm>
#include <cassert>
#include <cstring>

UIRenderer* UIRenderer::GetInstance()
{
	static UIRenderer instance;
	return &instance;
}

void UIRenderer::Initialize(DirectXCommon* dxCommon, DXCManager* dxc)
{
	dxCommon_ = dxCommon;
	vertices_.reserve(6 * 1024);
	CreateRootSignature();
	CreatePipeline(dxc);
	whiteTexture_ = TextureManager::GetInstance()->GetDefaultWhiteTexture();
	BuildFontAtlas();
}

void UIRenderer::Finalize()
{
	// Called after the GPU has been flushed (RizeEngine::Finalize)
	pso_.Reset();
	rootSignature_.Reset();
	Clear();
	dxCommon_ = nullptr;
}

void UIRenderer::CreateRootSignature()
{
	// [0] t0 texture (pixel), static linear-clamp sampler s0
	D3D12_DESCRIPTOR_RANGE range{};
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range.NumDescriptors = 1;
	range.BaseShaderRegister = 0;
	range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[1] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &range;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_STATIC_SAMPLER_DESC sampler{};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC sigDesc{};
	sigDesc.pParameters = rootParameters;
	sigDesc.NumParameters = _countof(rootParameters);
	sigDesc.pStaticSamplers = &sampler;
	sigDesc.NumStaticSamplers = 1;
	sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		if (errorBlob) {
			Log::GetInstance()->logOutput(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		}
		assert(false);
		return;
	}
	hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}

void UIRenderer::CreatePipeline(DXCManager* dxc)
{
	Microsoft::WRL::ComPtr<IDxcBlob> vsBlob = dxc->CompileShader(L"shaders/UI.VS.hlsl", L"vs_6_0", dxc->GetDxcUtils(), dxc->GetDxcCompiler(), dxc->GetIncludeHandler());
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob = dxc->CompileShader(L"shaders/UI.PS.hlsl", L"ps_6_0", dxc->GetDxcUtils(), dxc->GetDxcCompiler(), dxc->GetIncludeHandler());
	assert(vsBlob != nullptr && psBlob != nullptr);
	if (!vsBlob || !psBlob || !rootSignature_) {
		return;
	}

	D3D12_INPUT_ELEMENT_DESC inputElements[3] = {};
	inputElements[0].SemanticName = "POSITION";
	inputElements[0].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElements[1].SemanticName = "TEXCOORD";
	inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElements[2].SemanticName = "COLOR";
	inputElements[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_RASTERIZER_DESC rasterDesc{};
	rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterDesc.DepthClipEnable = TRUE;

	D3D12_DEPTH_STENCIL_DESC depthDesc{};
	depthDesc.DepthEnable = FALSE;
	depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	// Keep destination alpha (the editor shows the game render target through ImGui::Image)
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature_.Get();
	psoDesc.InputLayout = { inputElements, _countof(inputElements) };
	psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	psoDesc.RasterizerState = rasterDesc;
	psoDesc.DepthStencilState = depthDesc;
	psoDesc.BlendState = blendDesc;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_));
	assert(SUCCEEDED(hr));
	(void)hr;
}

void UIRenderer::BuildFontAtlas()
{
	// Rasterize ASCII 32..126 with GDI into a white-on-transparent atlas (no font file to ship)
	constexpr int kAtlasW = 512;
	constexpr int kAtlasH = 256;
	constexpr int kPad = 2;

	BITMAPINFO bmi{};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = kAtlasW;
	bmi.bmiHeader.biHeight = -kAtlasH; // top-down rows
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	HDC dc = CreateCompatibleDC(nullptr);
	void* bits = nullptr;
	HBITMAP bitmap = dc ? CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0) : nullptr;
	HFONT font = CreateFontW(-static_cast<int>(kFontPixelSize), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
	if (!dc || !bitmap || !bits || !font) {
		Log::GetInstance()->logOutput("[UIRenderer] Font atlas creation failed; text will not be drawn.\n");
		if (font) DeleteObject(font);
		if (bitmap) DeleteObject(bitmap);
		if (dc) DeleteDC(dc);
		return;
	}

	HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
	HGDIOBJ oldFont = SelectObject(dc, font);
	std::memset(bits, 0, static_cast<size_t>(kAtlasW) * kAtlasH * 4);
	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, RGB(255, 255, 255));

	TEXTMETRICW tm{};
	GetTextMetricsW(dc, &tm);
	const int lineH = (std::max)(1, static_cast<int>(tm.tmHeight));
	fontLineHeight_ = static_cast<float>(lineH);

	int penX = kPad;
	int penY = kPad;
	for (int c = 32; c < 128; ++c) {
		// 127 is not printable: draw a box glyph used for any unsupported character
		const wchar_t ch = (c == 127) ? L'\x25A1' : static_cast<wchar_t>(c);
		SIZE size{};
		GetTextExtentPoint32W(dc, &ch, 1, &size);
		if (penX + size.cx + kPad > kAtlasW) {
			penX = kPad;
			penY += lineH + kPad;
		}
		if (penY + lineH + kPad > kAtlasH) break; // atlas full (does not happen with this font size)
		TextOutW(dc, penX, penY, &ch, 1);

		Glyph& g = glyphs_[c - 32];
		g.u0 = static_cast<float>(penX) / kAtlasW;
		g.v0 = static_cast<float>(penY) / kAtlasH;
		g.u1 = static_cast<float>(penX + size.cx) / kAtlasW;
		g.v1 = static_cast<float>(penY + lineH) / kAtlasH;
		g.width = static_cast<float>(size.cx);
		g.advance = static_cast<float>(size.cx);
		penX += size.cx + kPad;
	}
	GdiFlush();

	// GDI writes coverage into RGB; move it to alpha with white color
	std::vector<uint8_t> rgba(static_cast<size_t>(kAtlasW) * kAtlasH * 4);
	const uint8_t* src = static_cast<const uint8_t*>(bits);
	for (size_t i = 0; i < static_cast<size_t>(kAtlasW) * kAtlasH; ++i) {
		const uint8_t coverage = (std::max)({ src[i * 4 + 0], src[i * 4 + 1], src[i * 4 + 2] });
		rgba[i * 4 + 0] = 255;
		rgba[i * 4 + 1] = 255;
		rgba[i * 4 + 2] = 255;
		rgba[i * 4 + 3] = coverage;
	}

	SelectObject(dc, oldFont);
	SelectObject(dc, oldBitmap);
	DeleteObject(font);
	DeleteObject(bitmap);
	DeleteDC(dc);

	fontTexture_ = TextureManager::GetInstance()->CreateTextureFromRGBA("UIRenderer/FontAtlas", rgba.data(), kAtlasW, kAtlasH);
}

void UIRenderer::Clear()
{
	vertices_.clear();
	batches_.clear();
}

void UIRenderer::PushQuad(uint32_t texture, float x, float y, float w, float h, float u0, float v0, float u1, float v1, const Vector4& color)
{
	if (vertices_.size() + 6 > kMaxVertices || w == 0.0f || h == 0.0f) return;

	if (batches_.empty() || batches_.back().texture != texture) {
		batches_.push_back({ texture, static_cast<uint32_t>(vertices_.size()), 0 });
	}

	// Canvas pixels -> NDC (y down in the canvas, up in NDC)
	const float l = x / kCanvasWidth * 2.0f - 1.0f;
	const float r = (x + w) / kCanvasWidth * 2.0f - 1.0f;
	const float t = 1.0f - y / kCanvasHeight * 2.0f;
	const float b = 1.0f - (y + h) / kCanvasHeight * 2.0f;

	vertices_.push_back({ l, t, u0, v0, color });
	vertices_.push_back({ r, t, u1, v0, color });
	vertices_.push_back({ l, b, u0, v1, color });
	vertices_.push_back({ l, b, u0, v1, color });
	vertices_.push_back({ r, t, u1, v0, color });
	vertices_.push_back({ r, b, u1, v1, color });
	batches_.back().vertexCount += 6;
}

void UIRenderer::DrawRect(float x, float y, float w, float h, const Vector4& color)
{
	PushQuad(whiteTexture_, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, color);
}

void UIRenderer::DrawImage(uint32_t textureHandle, float x, float y, float w, float h, const Vector4& color)
{
	PushQuad(textureHandle, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, color);
}

float UIRenderer::MeasureLine(const char* begin, const char* end, float scale) const
{
	float width = 0.0f;
	for (const char* p = begin; p < end; ++p) {
		const unsigned char c = static_cast<unsigned char>(*p);
		const Glyph& g = glyphs_[(c >= 32 && c < 127) ? c - 32 : 127 - 32];
		width += g.advance * scale;
	}
	return width;
}

float UIRenderer::MeasureText(const char* text, float size) const
{
	if (!text) return 0.0f;
	const float scale = size / fontLineHeight_;
	float widest = 0.0f;
	const char* lineStart = text;
	for (const char* p = text;; ++p) {
		if (*p == '\n' || *p == '\0') {
			widest = (std::max)(widest, MeasureLine(lineStart, p, scale));
			if (*p == '\0') break;
			lineStart = p + 1;
		}
	}
	return widest;
}

void UIRenderer::DrawString(const char* text, float x, float y, float size, const Vector4& color, Align align)
{
	if (!text || fontTexture_ == 0 || size <= 0.0f) return;
	const float scale = size / fontLineHeight_;
	float lineY = y;
	const char* lineStart = text;
	for (const char* p = text;; ++p) {
		if (*p != '\n' && *p != '\0') continue;

		float penX = x;
		if (align != Align::Left) {
			const float lineW = MeasureLine(lineStart, p, scale);
			penX -= (align == Align::Center) ? lineW * 0.5f : lineW;
		}
		for (const char* q = lineStart; q < p; ++q) {
			const unsigned char c = static_cast<unsigned char>(*q);
			const Glyph& g = glyphs_[(c >= 32 && c < 127) ? c - 32 : 127 - 32];
			if (c != ' ') {
				PushQuad(fontTexture_, penX, lineY, g.width * scale, size, g.u0, g.v0, g.u1, g.v1, color);
			}
			penX += g.advance * scale;
		}

		if (*p == '\0') break;
		lineStart = p + 1;
		lineY += size;
	}
}

void UIRenderer::Render(ID3D12GraphicsCommandList* cmdList)
{
	if (vertices_.empty() || !pso_ || !rootSignature_ || !dxCommon_ || dxCommon_->IsDeviceLost() || !cmdList) {
		return;
	}

	// Per-frame linear upload memory: rendering the same commands into several targets is safe
	const size_t bytes = sizeof(Vertex) * vertices_.size();
	DirectXCommon::UploadAllocation alloc = dxCommon_->AllocateUpload(bytes, 16);
	if (!alloc.cpu) return;
	std::memcpy(alloc.cpu, vertices_.data(), bytes);

	D3D12_VERTEX_BUFFER_VIEW vbView{};
	vbView.BufferLocation = alloc.gpu;
	vbView.SizeInBytes = static_cast<UINT>(bytes);
	vbView.StrideInBytes = sizeof(Vertex);

	TextureManager* textures = TextureManager::GetInstance();
	textures->SetDescriptorHeaps(cmdList);
	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->SetPipelineState(pso_.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView);
	for (const Batch& batch : batches_) {
		if (batch.vertexCount == 0) continue;
		cmdList->SetGraphicsRootDescriptorTable(0, textures->GetGPUHandle(batch.texture));
		cmdList->DrawInstanced(batch.vertexCount, 1, batch.firstVertex, 0);
		Profiler::GetInstance()->CountDrawCall();
	}
}
