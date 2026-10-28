#pragma once
#include "math/Matrix4x4.h"
#include "math/MathUtil.h"
#include "math/Quaternion.h"
#include "math/Vector3.h"
#include <cmath>

/// Transform helpers for the editor's GameObject hierarchy.
/// Engine convention: row vectors, world = Scale * Rotation * Translation * ParentWorld.
/// Quaternion::ToRotationMatrix4x4() produces the row-vector form, so the rotation of
/// (local then parent) is Quaternion::Multiply(parent, local).
namespace TransformUtil {

inline Matrix4x4 ComposeTRS(const Vector3& translation, const Quaternion& rotation, const Vector3& scale)
{
	return Mathematics::Multiply(Mathematics::MakeScaleMatrix(scale),
		Mathematics::Multiply(rotation.ToRotationMatrix4x4(), Mathematics::MakeTranslateMatrix(translation)));
}

/// Inverse of Quaternion::ToRotationMatrix4x4() for a pure rotation matrix (row-vector form).
inline Quaternion QuaternionFromRotationMatrix(const Matrix4x4& m)
{
	// m is the transpose of the column-vector matrix, so mirror the standard element pairs
	const float trace = m.m[0][0] + m.m[1][1] + m.m[2][2];
	Quaternion q{};
	if (trace > 0.0f) {
		const float s = 0.5f / std::sqrt(trace + 1.0f);
		q.w = 0.25f / s;
		q.x = (m.m[1][2] - m.m[2][1]) * s;
		q.y = (m.m[2][0] - m.m[0][2]) * s;
		q.z = (m.m[0][1] - m.m[1][0]) * s;
	} else if (m.m[0][0] > m.m[1][1] && m.m[0][0] > m.m[2][2]) {
		const float s = 2.0f * std::sqrt(1.0f + m.m[0][0] - m.m[1][1] - m.m[2][2]);
		q.w = (m.m[1][2] - m.m[2][1]) / s;
		q.x = 0.25f * s;
		q.y = (m.m[1][0] + m.m[0][1]) / s;
		q.z = (m.m[2][0] + m.m[0][2]) / s;
	} else if (m.m[1][1] > m.m[2][2]) {
		const float s = 2.0f * std::sqrt(1.0f + m.m[1][1] - m.m[0][0] - m.m[2][2]);
		q.w = (m.m[2][0] - m.m[0][2]) / s;
		q.x = (m.m[1][0] + m.m[0][1]) / s;
		q.y = 0.25f * s;
		q.z = (m.m[2][1] + m.m[1][2]) / s;
	} else {
		const float s = 2.0f * std::sqrt(1.0f + m.m[2][2] - m.m[0][0] - m.m[1][1]);
		q.w = (m.m[0][1] - m.m[1][0]) / s;
		q.x = (m.m[2][0] + m.m[0][2]) / s;
		q.y = (m.m[2][1] + m.m[1][2]) / s;
		q.z = 0.25f * s;
	}
	return Quaternion::Normalize(q);
}

/// Splits an affine matrix into translation, rotation and (positive) scale.
/// Shear from non-uniform parent scale combined with child rotation cannot be represented and is dropped.
inline void Decompose(const Matrix4x4& m, Vector3& outTranslation, Quaternion& outRotation, Vector3& outScale)
{
	outTranslation = { m.m[3][0], m.m[3][1], m.m[3][2] };
	auto rowLength = [&](int r) {
		return std::sqrt(m.m[r][0] * m.m[r][0] + m.m[r][1] * m.m[r][1] + m.m[r][2] * m.m[r][2]);
	};
	outScale = { rowLength(0), rowLength(1), rowLength(2) };

	Matrix4x4 rot = Mathematics::MakeIdentity4x4();
	const float s[3] = { outScale.x, outScale.y, outScale.z };
	for (int r = 0; r < 3; ++r) {
		const float inv = (s[r] > 1e-8f) ? 1.0f / s[r] : 0.0f;
		for (int c = 0; c < 3; ++c) rot.m[r][c] = m.m[r][c] * inv;
	}
	outRotation = QuaternionFromRotationMatrix(rot);
}

inline Vector3 TransformPoint(const Vector3& p, const Matrix4x4& m)
{
	return {
		p.x * m.m[0][0] + p.y * m.m[1][0] + p.z * m.m[2][0] + m.m[3][0],
		p.x * m.m[0][1] + p.y * m.m[1][1] + p.z * m.m[2][1] + m.m[3][1],
		p.x * m.m[0][2] + p.y * m.m[1][2] + p.z * m.m[2][2] + m.m[3][2],
	};
}

/// Direction (w = 0): ignores translation
inline Vector3 TransformDirection(const Vector3& d, const Matrix4x4& m)
{
	return {
		d.x * m.m[0][0] + d.y * m.m[1][0] + d.z * m.m[2][0],
		d.x * m.m[0][1] + d.y * m.m[1][1] + d.z * m.m[2][1],
		d.x * m.m[0][2] + d.y * m.m[1][2] + d.z * m.m[2][2],
	};
}

} // namespace TransformUtil
