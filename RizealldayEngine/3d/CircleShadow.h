#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"
#include "math/MathUtil.h"

class DirectXCommon;
class DXCManager;
class Camera;

/// Circle Shadow (Blob Shadow) Material for GPU
struct CircleShadowMaterialGPU {
    Vector4 color   = { 0.0f, 0.0f, 0.0f, 0.5f }; // Shadow color (black, 50% transparent)
    float   centerX = 0.5f;                          // UV center X
    float   centerY = 0.5f;                          // UV center Y
    float   radius  = 0.45f;                         // Shadow radius in UV space
    float   softness = 0.4f;                         // Shadow edge softness
};

/// CircleShadow - 3D blob shadow under characters/objects
/// Rendered as a flat quad with alpha blending
class CircleShadow
{
public:
    /// Initialize pipeline and static resources for circle shadow
    static void StaticInitialize(DirectXCommon* dxCommon, DXCManager* dxcManager);

    void Initialize();

    /// Update shadow position based on character position
    /// @param characterPos  Character world position
    /// @param floorY        Floor Y position where shadow is cast
    /// @param radius        World-space shadow radius
    /// @param opacity       Opacity (0=transparent, 1=fully dark)
    void Update(const Vector3& characterPos, float floorY, float radius, float opacity);

    void Draw(ID3D12GraphicsCommandList* cmdList, Camera* camera);

    /// Customize shadow color
    void SetColor(const Vector4& color);
    void SetSoftness(float softness) { if (materialData_) materialData_->softness = softness; }

    CircleShadowMaterialGPU* GetMaterial() { return materialData_; }

private:
    static DirectXCommon* sDxCommon_;
    static Microsoft::WRL::ComPtr<ID3D12PipelineState> sPipelineState_;
    static Microsoft::WRL::ComPtr<ID3D12RootSignature> sRootSignature_;
    static bool sInitialized_;

    static void CreateRootSignature();
    static void CreatePipelineState(DXCManager* dxc);

    // Per-instance resources
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> transformResource_;

    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    D3D12_INDEX_BUFFER_VIEW  ibView_{};

    CircleShadowMaterialGPU* materialData_  = nullptr;

    struct TransformMatrix { Matrix4x4 WVP; Matrix4x4 World; };
    TransformMatrix* transformData_ = nullptr;

    Vector3 position_ = {};
    float   radius_   = 1.0f;
    float   opacity_  = 0.5f;
};
