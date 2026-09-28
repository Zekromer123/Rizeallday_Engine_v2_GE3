#pragma once
#include "math/Vector3.h"
#include "math/Vector4.h"
#include <cstdint>

struct PointLightData {
	Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	Vector3 position = {0.0f, 0.0f, 0.0f};
	float intensity = 1.0f;
	float radius = 10.0f;
	float decay = 1.0f;
	int32_t active = 0;
	float pad = 0;
};
