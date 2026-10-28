#pragma once
#include "base/ScriptAPI.h"
#include <cmath>

// State shared by the Survivors sample scripts. Scripts live in the same DLL, so plain globals work
// (they reset on a script hot reload, which is fine for a sample).
namespace Survivors {
	inline int gKills = 0;
	inline int gPlayerHp = 0;
	inline int gPlayerMaxHp = 1;
	inline Vector3 gPlayerPos = { 0.0f, 0.0f, 0.0f };
	inline bool gGameOver = false;
	inline float gEnemySpeedScale = 1.0f;

	/// Yaw (rotation.y) that makes +Z face along (dx, dz)
	inline float YawFromDirection(float dx, float dz) { return std::atan2(dx, dz); }

	/// Writes rotation + rotationQuat together (both are read by the engine)
	inline void SetYaw(GameObject* obj, float yaw) {
		if (!obj) return;
		obj->rotation = { 0.0f, yaw, 0.0f };
		obj->rotationQuat = Quaternion::FromEuler(obj->rotation);
	}
}
