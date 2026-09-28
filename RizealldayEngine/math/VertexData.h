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
};

struct Material {
	Vector4 color;
	int32_t enableLighting;
	int32_t lightingType; // 0=None, 1=Lambert, 2=HalfLambert
	float padding[2];
};

struct MaterialData {
	std::string textureFilePath;
};

struct ModelData {
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;
	MaterialData material;
};
