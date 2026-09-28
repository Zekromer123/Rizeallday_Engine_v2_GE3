#pragma once
#include "math/Vector3.h"
#include "math/Matrix4x4.h"
#include "math/MathUtil.h"

class Camera {
public:
	void Initialize();
	void Update();

	const Matrix4x4& GetViewMatrix() const { return matView_; }
	const Matrix4x4& GetProjectionMatrix() const { return matProjection_; }
	const Matrix4x4& GetViewProjectionMatrix() const { return matViewProjection_; }

	Vector3 translation = {0.0f, 0.0f, -10.0f};
	Vector3 rotation = {0.0f, 0.0f, 0.0f};

	float fovY = 0.45f;
	float aspectRatio = 1280.0f / 720.0f;
	float nearClip = 0.1f;
	float farClip = 1000.0f;

private:
	Matrix4x4 matView_ = {};
	Matrix4x4 matProjection_ = {};
	Matrix4x4 matViewProjection_ = {};
};
