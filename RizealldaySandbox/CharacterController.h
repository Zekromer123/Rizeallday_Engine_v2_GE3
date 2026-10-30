#pragma once
#include "math/Vector3.h"
#include <vector>

/// Kinematic character movement against axis-aligned boxes (the engine's colliders): slide along walls,
/// step onto low ledges, land on floors, never tunnel through thin walls. Pure math, no editor state.
namespace CharacterController {
	struct Box {
		Vector3 min{};
		Vector3 max{};
	};

	struct Result {
		Vector3 moved{};         // displacement actually applied
		bool grounded = false;   // standing on something after the move
		bool hitWall = false;    // horizontal motion was blocked
		bool hitCeiling = false; // upward motion was blocked
	};

	/// Gap left between the character and what it touches, so the next move does not start inside it
	inline constexpr float kSkin = 0.001f;
	/// How far below the feet still counts as standing
	inline constexpr float kGroundProbe = 0.05f;

	/// Moves `self` by `motion` (X, Z, then Y per sub-step). Ledges up to `stepHeight` are climbed while grounded.
	Result Move(const Box& self, const Vector3& motion, const std::vector<Box>& obstacles, float stepHeight);
}
