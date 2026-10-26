#pragma once
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"
#include <cmath>
#include <algorithm>

struct AABB {
	Vector3 min = { -0.5f, -0.5f, -0.5f };
	Vector3 max = {  0.5f,  0.5f,  0.5f };
};

struct Frustum {
	// 6 planes: 0=Left, 1=Right, 2=Bottom, 3=Top, 4=Near, 5=Far
	// Plane equation: a*x + b*y + c*z + d >= 0 is inside the frustum
	Vector4 planes[6]{};

	static Frustum ExtractFromViewProjection(const Matrix4x4& vp) {
		Frustum f{};
		// Row-vector convention (v_clip = v_world * VP):
		// clip.x = v.x*m[0][0] + v.y*m[1][0] + v.z*m[2][0] + m[3][0]
		// clip.y = v.x*m[0][1] + v.y*m[1][1] + v.z*m[2][1] + m[3][1]
		// clip.z = v.x*m[0][2] + v.y*m[1][2] + v.z*m[2][2] + m[3][2]
		// clip.w = v.x*m[0][3] + v.y*m[1][3] + v.z*m[2][3] + m[3][3]
		// Left:   clip.x + clip.w >= 0
		f.planes[0] = { vp.m[0][3] + vp.m[0][0], vp.m[1][3] + vp.m[1][0], vp.m[2][3] + vp.m[2][0], vp.m[3][3] + vp.m[3][0] };
		// Right:  clip.w - clip.x >= 0
		f.planes[1] = { vp.m[0][3] - vp.m[0][0], vp.m[1][3] - vp.m[1][0], vp.m[2][3] - vp.m[2][0], vp.m[3][3] - vp.m[3][0] };
		// Bottom: clip.y + clip.w >= 0
		f.planes[2] = { vp.m[0][3] + vp.m[0][1], vp.m[1][3] + vp.m[1][1], vp.m[2][3] + vp.m[2][1], vp.m[3][3] + vp.m[3][1] };
		// Top:    clip.w - clip.y >= 0
		f.planes[3] = { vp.m[0][3] - vp.m[0][1], vp.m[1][3] - vp.m[1][1], vp.m[2][3] - vp.m[2][1], vp.m[3][3] - vp.m[3][1] };
		// Near:   clip.z >= 0 (DirectX [0, 1] depth range)
		f.planes[4] = { vp.m[0][2], vp.m[1][2], vp.m[2][2], vp.m[3][2] };
		// Far:    clip.w - clip.z >= 0
		f.planes[5] = { vp.m[0][3] - vp.m[0][2], vp.m[1][3] - vp.m[1][2], vp.m[2][3] - vp.m[2][2], vp.m[3][3] - vp.m[3][2] };

		for (int i = 0; i < 6; ++i) {
			float len = std::sqrt(f.planes[i].x * f.planes[i].x + f.planes[i].y * f.planes[i].y + f.planes[i].z * f.planes[i].z);
			if (len > 0.00001f) {
				f.planes[i].x /= len;
				f.planes[i].y /= len;
				f.planes[i].z /= len;
				f.planes[i].w /= len;
			}
		}
		return f;
	}

	// Transform a local-space AABB into a world-space AABB using an affine World matrix
	static AABB TransformAABB(const AABB& localBox, const Matrix4x4& worldMat) {
		Vector3 corners[8] = {
			{ localBox.min.x, localBox.min.y, localBox.min.z },
			{ localBox.max.x, localBox.min.y, localBox.min.z },
			{ localBox.min.x, localBox.max.y, localBox.min.z },
			{ localBox.max.x, localBox.max.y, localBox.min.z },
			{ localBox.min.x, localBox.min.y, localBox.max.z },
			{ localBox.max.x, localBox.min.y, localBox.max.z },
			{ localBox.min.x, localBox.max.y, localBox.max.z },
			{ localBox.max.x, localBox.max.y, localBox.max.z }
		};

		AABB worldBox{ { 1e9f, 1e9f, 1e9f }, { -1e9f, -1e9f, -1e9f } };
		for (int i = 0; i < 8; ++i) {
			float wx = corners[i].x * worldMat.m[0][0] + corners[i].y * worldMat.m[1][0] + corners[i].z * worldMat.m[2][0] + worldMat.m[3][0];
			float wy = corners[i].x * worldMat.m[0][1] + corners[i].y * worldMat.m[1][1] + corners[i].z * worldMat.m[2][1] + worldMat.m[3][1];
			float wz = corners[i].x * worldMat.m[0][2] + corners[i].y * worldMat.m[1][2] + corners[i].z * worldMat.m[2][2] + worldMat.m[3][2];
			worldBox.min.x = (std::min)(worldBox.min.x, wx);
			worldBox.min.y = (std::min)(worldBox.min.y, wy);
			worldBox.min.z = (std::min)(worldBox.min.z, wz);
			worldBox.max.x = (std::max)(worldBox.max.x, wx);
			worldBox.max.y = (std::max)(worldBox.max.y, wy);
			worldBox.max.z = (std::max)(worldBox.max.z, wz);
		}
		return worldBox;
	}

	bool IsAABBVisible(const AABB& worldBox) const {
		for (int i = 0; i < 6; ++i) {
			const Vector4& p = planes[i];
			// Positive vertex (p-vertex) along plane normal
			Vector3 pv = {
				(p.x >= 0.0f) ? worldBox.max.x : worldBox.min.x,
				(p.y >= 0.0f) ? worldBox.max.y : worldBox.min.y,
				(p.z >= 0.0f) ? worldBox.max.z : worldBox.min.z
			};
			if (p.x * pv.x + p.y * pv.y + p.z * pv.z + p.w < 0.0f) {
				return false; // Completely outside this frustum plane
			}
		}
		return true;
	}
};
