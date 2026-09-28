#pragma once
#include <cstdint>
#include <vector>
#include <string>

struct Vector4
{
	float x, y, z, w;
};
struct Vector3
{
	float x, y, z;
};
struct Vector2
{
	float x, y;
};
struct VertexData
{
	Vector4 position; // x,y,z,w
	Vector2 texcoord; //u,v coordinate
	Vector3 normal;
};
struct Material
{
	Vector4 color;
	int32_t enableLighting;
};
struct MaterialData {
	std::string textureFilePath;
};

struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};