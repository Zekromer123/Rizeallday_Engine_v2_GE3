#include "CharacterController.h"
#include <algorithm>
#include <cmath>

namespace CharacterController {
namespace {
	float& Axis(Vector3& v, int axis) { return axis == 0 ? v.x : (axis == 1 ? v.y : v.z); }
	float Axis(const Vector3& v, int axis) { return axis == 0 ? v.x : (axis == 1 ? v.y : v.z); }

	// Touching faces do not overlap, so a character resting on a floor can still move along it
	bool Overlaps(const Box& a, const Box& b)
	{
		return a.min.x < b.max.x && a.max.x > b.min.x && a.min.y < b.max.y && a.max.y > b.min.y && a.min.z < b.max.z && a.max.z > b.min.z;
	}

	Box Shifted(Box b, int axis, float delta)
	{
		Axis(b.min, axis) += delta;
		Axis(b.max, axis) += delta;
		return b;
	}
}

Result Move(const Box& self, const Vector3& motion, const std::vector<Box>& obstacles, float stepHeight)
{
	// Obstacles the character already overlaps never block it, so it can walk out of them
	std::vector<const Box*> solid;
	solid.reserve(obstacles.size());
	for (const Box& o : obstacles) {
		if (!Overlaps(self, o)) solid.push_back(&o);
	}
	auto blocked = [&](const Box& b) {
		return std::any_of(solid.begin(), solid.end(), [&](const Box* o) { return Overlaps(b, *o); });
	};
	auto onGround = [&](const Box& b) {
		Box feet = b;
		feet.max.y = b.min.y;
		feet.min.y = b.min.y - kGroundProbe;
		return blocked(feet);
	};

	Result result;
	Box box = self;
	bool grounded = onGround(box);

	// Each sub-step is at most half the character's smallest size, so it cannot skip past any obstacle
	const float smallest = (std::min)({ self.max.x - self.min.x, self.max.y - self.min.y, self.max.z - self.min.z });
	const float maxStep = (std::max)(smallest * 0.5f, 0.01f);
	const float longest = (std::max)({ std::abs(motion.x), std::abs(motion.y), std::abs(motion.z) });
	const int steps = std::clamp(static_cast<int>(std::ceil(longest / maxStep)), 1, 1000);

	// Returns true when the axis was blocked
	auto moveAxis = [&](int axis, float delta) {
		if (delta == 0.0f) return false;
		const Box moved = Shifted(box, axis, delta);
		float limit = delta > 0.0f ? 1e30f : -1e30f;
		float highestTop = -1e30f;
		bool hit = false;
		for (const Box* o : solid) {
			if (!Overlaps(moved, *o)) continue;
			hit = true;
			limit = delta > 0.0f ? (std::min)(limit, Axis(o->min, axis)) : (std::max)(limit, Axis(o->max, axis));
			highestTop = (std::max)(highestTop, o->max.y);
		}
		if (!hit) {
			box = moved;
			return false;
		}
		// Step onto a low ledge: lift first, then move, both in free space
		if (axis != 1 && grounded && stepHeight > 0.0f) {
			const float rise = highestTop - box.min.y + kSkin;
			if (rise > 0.0f && rise <= stepHeight + kSkin) {
				const Box lifted = Shifted(box, 1, rise);
				const Box stepped = Shifted(moved, 1, rise);
				if (!blocked(lifted) && !blocked(stepped)) {
					box = stepped;
					return false;
				}
			}
		}
		// Stop just short of the nearest face (never move backwards)
		const float allowed = delta > 0.0f
			? std::clamp(limit - kSkin - Axis(box.max, axis), 0.0f, delta)
			: std::clamp(limit + kSkin - Axis(box.min, axis), delta, 0.0f);
		box = Shifted(box, axis, allowed);
		return true;
	};

	const Vector3 step = { motion.x / steps, motion.y / steps, motion.z / steps };
	for (int i = 0; i < steps; ++i) {
		result.hitWall |= moveAxis(0, step.x);
		result.hitWall |= moveAxis(2, step.z);
		if (moveAxis(1, step.y)) {
			if (step.y < 0.0f) grounded = true;
			else result.hitCeiling = true;
		}
		grounded = onGround(box);
	}

	result.moved = { box.min.x - self.min.x, box.min.y - self.min.y, box.min.z - self.min.z };
	result.grounded = grounded;
	return result;
}

}
