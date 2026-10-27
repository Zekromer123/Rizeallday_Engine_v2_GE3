#include "SpawnStressTest.h"
#include <cstdio>

void SpawnStressTest::Initialize() {
	elapsed_ = 0.0f;
	trackedHead_ = 0;
	trackedCount_ = 0;
	totalSpawned_ = 0;
	totalCancelled_ = 0;
	totalDestroyed_ = 0;
	finished_ = false;
	Log("[SpawnStressTest] Started.", 0);
}

void SpawnStressTest::Update(float deltaTime) {
	if (!testEnabled || finished_) return;

	elapsed_ += deltaTime;
	DestroyExpired(false);

	if (elapsed_ >= testDuration) {
		DestroyExpired(true);
		finished_ = true;
		char msg[256]{};
		std::snprintf(msg, sizeof(msg), "[SpawnStressTest] PASSED: spawned %d, cancelled %d, destroyed %d in %.1fs.",
			totalSpawned_, totalCancelled_, totalDestroyed_, elapsed_);
		Log(msg, 0);
		return;
	}

	const Vector3 origin = GetPosition();
	for (int n = 0; n < spawnPerFrame; ++n) {
		const float x = origin.x + static_cast<float>((totalSpawned_ * 7) % 21) - 10.0f;
		const float z = origin.z + static_cast<float>((totalSpawned_ * 13) % 21) - 10.0f;
		GameObject* cube = Instantiate("StressCube", 0, Vector3{ x, origin.y + 5.0f, z });
		if (!cube) {
			Log("[SpawnStressTest] FAILED: Instantiate returned nullptr.", 2);
			finished_ = true;
			return;
		}
		++totalSpawned_;

		// Write through the view before the object exists, then look it up again by id
		cube->scale = Vector3{ 0.25f, 0.25f, 0.25f };
		if (FindGameObjectById(cube->id) != cube) {
			Log("[SpawnStressTest] FAILED: pending object lookup returned a different view.", 2);
			finished_ = true;
			return;
		}

		// Every 10th spawn is cancelled in the same frame
		if (n % 10 == 9) {
			DestroyGameObject(cube->id);
			++totalCancelled_;
			continue;
		}

		if (trackedCount_ >= kMaxTracked) {
			DestroyGameObject(cube->id);
			++totalDestroyed_;
			continue;
		}
		const int slot = (trackedHead_ + trackedCount_) % kMaxTracked;
		tracked_[slot].id = cube->id;
		tracked_[slot].spawnTime = elapsed_;
		++trackedCount_;
	}

	// The owner view must still be valid after spawning (it used to dangle)
	if (GameObject* self = GetOwner()) {
		self->rotation.y += deltaTime;
		self->rotationQuat = Quaternion::FromEuler(self->rotation);
		self->dirtyFromScript = true;
	}
}

void SpawnStressTest::OnDestroy() {
	DestroyExpired(true);
}

void SpawnStressTest::DestroyExpired(bool destroyAll) {
	while (trackedCount_ > 0) {
		const TrackedCube& oldest = tracked_[trackedHead_];
		if (!destroyAll && (elapsed_ - oldest.spawnTime) < cubeLifetime) {
			break;
		}
		DestroyGameObject(oldest.id);
		++totalDestroyed_;
		trackedHead_ = (trackedHead_ + 1) % kMaxTracked;
		--trackedCount_;
	}
}

REGISTER_SCRIPT(SpawnStressTest)
