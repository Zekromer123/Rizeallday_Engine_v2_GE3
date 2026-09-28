#pragma once
#include <cmath>

struct Vector3 {
	float x, y, z;
	Vector3() : x(0), y(0), z(0) {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
	Vector3 operator+(const Vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
	Vector3 operator-(const Vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
	Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
	Vector3 operator/(float s) const { return {x / s, y / s, z / s}; }
	Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
	Vector3& operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	Vector3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
	float Length() const { return std::sqrtf(x * x + y * y + z * z); }
	Vector3 Normalize() const { float len = Length(); if (len == 0) return {0,0,0}; return {x/len, y/len, z/len}; }
};

inline float Dot(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vector3 Cross(const Vector3& a, const Vector3& b) { return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x}; }
inline Vector3 Normalize(const Vector3& v) { return v.Normalize(); }
