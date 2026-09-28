#pragma once
#include "math/Vector3.h"
#include "math/MathUtil.h"
#include "input/Input.h" 
#include <cstdint>
class DebugCamera
{
public:
	void Initialize();
	void Update(Input* input);

	//Getter
	const Matrix4x4& GetViewMatrix() const { return matView_; }
	const Matrix4x4& GetProjectionMatrix() const { return matProjection_; }

private:

	Matrix4x4 matRot_ = {};

	Vector3 translation_ = { 0.0f, 0.0f, -50.0f };   // Camera Position

	Matrix4x4 matView_ = {}; 
	Matrix4x4 matProjection_ = {};

};


