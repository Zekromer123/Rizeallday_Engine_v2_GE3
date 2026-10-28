#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <vector>
#include "math/Vector4.h"

class DirectXCommon;
class DXCManager;

/// Immediate-mode game UI (HUD, menus). Works in every build configuration, including Release without ImGui.
/// Coordinates are in a fixed 1280 x 720 canvas (origin top-left) that is stretched over the game view.
/// Commands are kept until Clear(), so the same UI can be rendered into several targets in one frame.
class UIRenderer
{
public:
	static constexpr float kCanvasWidth = 1280.0f;
	static constexpr float kCanvasHeight = 720.0f;
	static constexpr float kFontPixelSize = 32.0f; // atlas glyph height; text is scaled from this

	enum class Align : int { Left = 0, Center = 1, Right = 2 };

	static UIRenderer* GetInstance();

	void Initialize(DirectXCommon* dxCommon, DXCManager* dxc);
	void Finalize();

	void Clear();
	bool HasCommands() const { return !vertices_.empty(); }
	size_t GetQuadCount() const { return vertices_.size() / 6; }

	void DrawRect(float x, float y, float w, float h, const Vector4& color);
	void DrawImage(uint32_t textureHandle, float x, float y, float w, float h, const Vector4& color);
	/// '\n' starts a new line. `size` is the line height in canvas pixels.
	void DrawString(const char* text, float x, float y, float size, const Vector4& color, Align align = Align::Left);
	/// Width of the widest line of `text` at `size`
	float MeasureText(const char* text, float size) const;

	/// Draws all queued commands into the currently bound render target (sRGB color + D24 depth formats)
	void Render(ID3D12GraphicsCommandList* cmdList);

private:
	UIRenderer() = default;
	~UIRenderer() = default;
	UIRenderer(const UIRenderer&) = delete;
	UIRenderer& operator=(const UIRenderer&) = delete;

	struct Vertex {
		float x, y;   // NDC
		float u, v;
		Vector4 color;
	};
	struct Batch {
		uint32_t texture = 0;
		uint32_t firstVertex = 0;
		uint32_t vertexCount = 0;
	};
	struct Glyph {
		float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
		float width = 0;   // pixels at kFontPixelSize
		float advance = 0;
	};

	void CreateRootSignature();
	void CreatePipeline(DXCManager* dxc);
	void BuildFontAtlas();
	void PushQuad(uint32_t texture, float x, float y, float w, float h, float u0, float v0, float u1, float v1, const Vector4& color);
	float MeasureLine(const char* begin, const char* end, float scale) const;

	static constexpr size_t kMaxVertices = 6 * 16384;

	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;
	std::vector<Vertex> vertices_;
	std::vector<Batch> batches_;
	Glyph glyphs_[96]{}; // ASCII 32..127 (127 = fallback box)
	uint32_t fontTexture_ = 0;
	uint32_t whiteTexture_ = 0;
	float fontLineHeight_ = kFontPixelSize;
};
