#pragma once
#include "math/Vector3.h"
#include "math/Vector4.h"
#include <cstdint>

struct DirectionalLightData {
	Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	Vector3 direction = {0.0f, -1.0f, 0.0f};
	float intensity = 1.0f;
	int32_t active = 1;
	float pad[3] = {};
};
