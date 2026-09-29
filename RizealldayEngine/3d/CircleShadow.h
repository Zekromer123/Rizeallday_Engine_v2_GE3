#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"
#include "math/MathUtil.h"
#include "3d/Camera.h"

class DirectXCommon;
class DXCManager;

/// Circle Shadow (Blob Shadow) Material untuk GPU
struct CircleShadowMaterialGPU {
    Vector4 color   = { 0.0f, 0.0f, 0.0f, 0.5f }; // Warna bayangan (hitam, 50% transparan)
    float   centerX = 0.5f;                          // Pusat UV X
    float   centerY = 0.5f;                          // Pusat UV Y
    float   radius  = 0.45f;                         // Radius bayangan dalam UV space
    float   softness = 0.4f;                         // Kelembutan tepi bayangan
};

/// CircleShadow - Bayangan lingkaran 3D di bawah karakter
/// Dirender sebagai quad datar dengan alpha blending
class CircleShadow
{
public:
    /// Inisialisasi pipeline dan resource statis untuk circle shadow.
    static void StaticInitialize(DirectXCommon* dxCommon, DXCManager* dxcManager);

    void Initialize();

    /// Update posisi bayangan berdasarkan posisi karakter.
    /// @param characterPos  Posisi karakter di dunia
    /// @param floorY        Ketinggian lantai (Y dimana bayangan ditempatkan)
    /// @param radius        Radius bayangan dalam satuan dunia
    /// @param opacity       Transparansi (0=tembus, 1=gelap penuh)
    void Update(const Vector3& characterPos, float floorY, float radius, float opacity);

    void Draw(ID3D12GraphicsCommandList* cmdList, Camera* camera);

    /// Kustomisasi warna bayangan
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
