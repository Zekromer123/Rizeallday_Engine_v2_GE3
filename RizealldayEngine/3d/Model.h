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

struct BoneInfo {
	std::string name;
	uint32_t id = 0;
	Matrix4x4 offsetMatrix;
};

struct KeyVector3 {
	float time = 0.0f;
	Vector3 value = { 0.0f, 0.0f, 0.0f };
};

struct KeyQuat {
	float time = 0.0f;
	float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
};

struct NodeAnimTrack {
	std::vector<KeyVector3> positions;
	std::vector<KeyQuat> rotations;
	std::vector<KeyVector3> scalings;
};

struct AnimationClip {
	std::string name;
	float duration = 0.0f; // in ticks
	float ticksPerSecond = 25.0f;
	std::unordered_map<std::string, NodeAnimTrack> channels;
};

struct NodeData {
	std::string name;
	Matrix4x4 transform;
	Vector3 defaultPosition = { 0.0f, 0.0f, 0.0f };
	KeyQuat defaultRotation = { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	Vector3 defaultScaling = { 1.0f, 1.0f, 1.0f };
	std::vector<NodeData> children;
};

class Model {
public:
	Model() = default;
	~Model();
	static void StaticInitialize(DirectXCommon* dxCommon);
	static Model* CreateFromFile(const std::string& directoryPath, const std::string& filename);
	static std::unique_ptr<Model> CreatePreviewFromFile(const std::string& filePath);
	static Model* CreateCube();
	static Model* CreateSphere();
	static Model* CreatePlane();
	static Model* CreateQuad();
	
	void Draw(ID3D12GraphicsCommandList* cmdList, uint32_t overrideTextureHandle = 0);
	const std::vector<MeshData>& GetMeshes() const { return meshes_; }
	uint32_t GetTotalVertexCount() const;
	uint32_t GetTotalTriangleCount() const;
	void GetBounds(Vector3& outCenter, float& outMaxExtent) const;

	bool HasBones() const { return !boneInfoMap_.empty(); }
	size_t GetBoneCount() const { return boneInfoMap_.size(); }
	const std::vector<BoneInfo>& GetBoneList() const { return boneList_; }
	size_t GetAnimationCount() const { return animations_.size(); }
	float GetAnimationDuration(int animIndex = 0) const;

	void CalculateBoneTransforms(float timeInSeconds, int animIndex, std::vector<Matrix4x4>& outMatrices) const;

	void SetLockRootMotion(bool lock) { lockRootMotion_ = lock; }
	bool GetLockRootMotion() const { return lockRootMotion_; }

private:
	void LoadAssimpModel(const std::string& filePath);
	void LoadOBJ(const std::string& directoryPath, const std::string& filename);
	void CreateGPUBuffers();

	NodeData BuildNodeHierarchy(void* pAiNode);
	void ReadNodeHierarchy(float animTimeTicks, const AnimationClip* pAnim, const NodeData& node, const Matrix4x4& parentTransform, std::vector<Matrix4x4>& outMatrices) const;
	
	static Vector3 InterpolatePosition(float animTimeTicks, const NodeAnimTrack& track, const Vector3& defaultPos);
	static KeyQuat InterpolateRotation(float animTimeTicks, const NodeAnimTrack& track, const KeyQuat& defaultRot);
	static Vector3 InterpolateScaling(float animTimeTicks, const NodeAnimTrack& track, const Vector3& defaultScale);

	std::vector<MeshData> meshes_;
	std::unordered_map<std::string, BoneInfo> boneInfoMap_;
	std::vector<BoneInfo> boneList_;
	NodeData rootNode_;
	Matrix4x4 globalInverseTransform_ = Mathematics::MakeIdentity4x4();
	std::vector<AnimationClip> animations_;
	bool lockRootMotion_ = false;
	
	static DirectXCommon* sDxCommon_;

	static std::unordered_map<std::string, std::unique_ptr<Model>> sCache_;
};
