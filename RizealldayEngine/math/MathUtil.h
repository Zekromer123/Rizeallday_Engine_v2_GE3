#pragma once
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"

class Mathematics
{
public:
	static Matrix4x4 MakeIdentity4x4();
	static Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);
	static Matrix4x4 MakeScaleMatrix(const Vector3& scale);
	static Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);
	static Matrix4x4 MakeRotateXMatrix(float radian);
	static Matrix4x4 MakeRotateYMatrix(float radian);
	static Matrix4x4 MakeRotateZMatrix(float radian);
	static Matrix4x4 Multiply(Matrix4x4 m1, Matrix4x4 m2);
	static Matrix4x4 MakeTranslateMatrix(const Vector3& translate);
	static Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspect, float nearClip, float farClip);
	static Matrix4x4 Inverse(const Matrix4x4& m);
	static Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);
};
