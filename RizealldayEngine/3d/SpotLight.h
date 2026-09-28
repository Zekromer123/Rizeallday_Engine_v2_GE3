#pragma once
#include "math/Vector3.h"
#include "math/Vector4.h"
#include <cmath>
#include <cstdint>

struct SpotLightData {
	Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	Vector3 position = {0.0f, 0.0f, 0.0f};
	float intensity = 1.0f;
	Vector3 direction = {0.0f, -1.0f, 0.0f};
	float distance = 10.0f;
	float decay = 1.0f;
	float cosAngle = 0.7071f;   // cos(45 deg)
	float cosFalloffStart = 0.5f; // cos(60 deg)
	int32_t active = 0;
};
