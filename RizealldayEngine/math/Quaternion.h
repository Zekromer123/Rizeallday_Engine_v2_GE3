#pragma once
#include <cmath>
#include <algorithm>
#include "math/Vector3.h"
#include "math/Matrix4x4.h"

struct Quaternion {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float w = 1.0f;

	constexpr Quaternion() = default;
	constexpr Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

	static Quaternion Identity() {
		return { 0.0f, 0.0f, 0.0f, 1.0f };
	}

	static Quaternion FromEuler(const Vector3& euler) {
		float p = euler.x * 0.5f;
		float y = euler.y * 0.5f;
		float r = euler.z * 0.5f;
		float sinP = std::sin(p);
		float cosP = std::cos(p);
		float sinY = std::sin(y);
		float cosY = std::cos(y);
		float sinR = std::sin(r);
		float cosR = std::cos(r);

		Quaternion q;
		q.w = cosY * cosP * cosR + sinY * sinP * sinR;
		q.x = cosY * sinP * cosR + sinY * cosP * sinR;
		q.y = sinY * cosP * cosR - cosY * sinP * sinR;
		q.z = cosY * cosP * sinR - sinY * sinP * cosR;
		return Normalize(q);
	}

	Vector3 ToEuler() const {
		Vector3 euler;
		// Roll (Z)
		float sinr_cosp = 2.0f * (w * z + x * y);
		float cosr_cosp = 1.0f - 2.0f * (y * y + z * z);
		euler.z = std::atan2(sinr_cosp, cosr_cosp);

		// Pitch (X)
		float sinp = 2.0f * (w * x - y * z);
		if (std::abs(sinp) >= 1.0f) {
			euler.x = std::copysign(3.14159265358979323846f / 2.0f, sinp);
		} else {
			euler.x = std::asin(sinp);
		}

		// Yaw (Y)
		float siny_cosp = 2.0f * (w * y + z * x);
		float cosy_cosp = 1.0f - 2.0f * (x * x + y * y);
		euler.y = std::atan2(siny_cosp, cosy_cosp);

		return euler;
	}

	static Quaternion FromAxisAngle(const Vector3& axis, float angle) {
		float len = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
		Vector3 norm = (len > 0.000001f) ? Vector3{ axis.x / len, axis.y / len, axis.z / len } : axis;
		float half = angle * 0.5f;
		float s = std::sin(half);
		return { norm.x * s, norm.y * s, norm.z * s, std::cos(half) };
	}

	static Quaternion Multiply(const Quaternion& q1, const Quaternion& q2) {
		return {
			q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
			q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
			q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
			q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z
		};
	}

	static Quaternion Normalize(const Quaternion& q) {
		float mag = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
		if (mag <= 0.000001f) return Identity();
		return { q.x / mag, q.y / mag, q.z / mag, q.w / mag };
	}

	static Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float t) {
		Quaternion end = q2;
		float dot = q1.x * end.x + q1.y * end.y + q1.z * end.z + q1.w * end.w;
		if (dot < 0.0f) {
			dot = -dot;
			end = { -end.x, -end.y, -end.z, -end.w };
		}

		if (dot > 0.9995f) {
			Quaternion res = {
				q1.x + t * (end.x - q1.x),
				q1.y + t * (end.y - q1.y),
				q1.z + t * (end.z - q1.z),
				q1.w + t * (end.w - q1.w)
			};
			return Normalize(res);
		}

		float theta_0 = std::acos(std::clamp(dot, -1.0f, 1.0f));
		float theta = theta_0 * t;
		float sin_theta = std::sin(theta);
		float sin_theta_0 = std::sin(theta_0);
		float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
		float s1 = sin_theta / sin_theta_0;

		return {
			(s0 * q1.x) + (s1 * end.x),
			(s0 * q1.y) + (s1 * end.y),
			(s0 * q1.z) + (s1 * end.z),
			(s0 * q1.w) + (s1 * end.w)
		};
	}

	Quaternion Normalize() const {
		return Normalize(*this);
	}

	Quaternion Multiply(const Quaternion& other) const {
		return Multiply(*this, other);
	}

	Quaternion Slerp(const Quaternion& other, float t) const {
		return Slerp(*this, other, t);
	}

	Matrix4x4 ToRotationMatrix4x4() const {
		Matrix4x4 m{};
		float xx = x * x, yy = y * y, zz = z * z;
		float xy = x * y, xz = x * z, yz = y * z;
		float wx = w * x, wy = w * y, wz = w * z;

		m.m[0][0] = 1.0f - 2.0f * (yy + zz);
		m.m[0][1] = 2.0f * (xy + wz);
		m.m[0][2] = 2.0f * (xz - wy);
		m.m[0][3] = 0.0f;

		m.m[1][0] = 2.0f * (xy - wz);
		m.m[1][1] = 1.0f - 2.0f * (xx + zz);
		m.m[1][2] = 2.0f * (yz + wx);
		m.m[1][3] = 0.0f;

		m.m[2][0] = 2.0f * (xz + wy);
		m.m[2][1] = 2.0f * (yz - wx);
		m.m[2][2] = 1.0f - 2.0f * (xx + yy);
		m.m[2][3] = 0.0f;

		m.m[3][0] = 0.0f;
		m.m[3][1] = 0.0f;
		m.m[3][2] = 0.0f;
		m.m[3][3] = 1.0f;
		return m;
	}
};
