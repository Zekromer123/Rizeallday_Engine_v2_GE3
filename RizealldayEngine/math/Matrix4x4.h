#pragma once
#include "math/Vector3.h"

struct Matrix4x4 {
	float m[4][4];
};

struct Transform {
	Vector3 scale = { 1.0f, 1.0f, 1.0f };
	Vector3 rotate = { 0.0f, 0.0f, 0.0f };
	Vector3 translate = { 0.0f, 0.0f, 0.0f };
};

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
};
