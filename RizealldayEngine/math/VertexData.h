#pragma once
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include <string>
#include <vector>
#include <cstdint>

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
	uint32_t boneIndices[4] = { 0, 0, 0, 0 };
	float boneWeights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct Material {
	Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	int32_t enableLighting = 1;
	int32_t lightingType = 4; // 0=Unlit, 1=Lambert, 2=HalfLambert, 3=Phong, 4=BlinnPhong
	float shininess = 32.0f;
	int32_t receiveShadows = 1;
	Vector3 specularColor = { 1.0f, 1.0f, 1.0f };
	float explodeAmount = 0.0f;
	Vector2 uvOffset = { 0.0f, 0.0f };
	Vector2 uvTiling = { 1.0f, 1.0f };
	float uvRotation = 0.0f;
	float pad[3] = {};
};

struct MaterialData {
	std::string textureFilePath;
};

struct ModelData {
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;
	MaterialData material;
};
