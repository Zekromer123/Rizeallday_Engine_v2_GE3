#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "math/VertexData.h"
#include "math/MathUtil.h"

class DirectXCommon;
class TextureManager;

struct MeshData {
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;
	uint32_t textureHandle = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	D3D12_INDEX_BUFFER_VIEW ibView{};
};

class Model {
public:
	static void StaticInitialize(DirectXCommon* dxCommon);
	static Model* CreateFromFile(const std::string& directoryPath, const std::string& filename);
	
	void Draw(ID3D12GraphicsCommandList* cmdList);
	const std::vector<MeshData>& GetMeshes() const { return meshes_; }

private:
	void LoadAssimpModel(const std::string& filePath);
	void LoadOBJ(const std::string& directoryPath, const std::string& filename);
	void CreateGPUBuffers();
	
	std::vector<MeshData> meshes_;
	
	static DirectXCommon* sDxCommon_;
	static std::unordered_map<std::string, std::unique_ptr<Model>> sCache_;
};
