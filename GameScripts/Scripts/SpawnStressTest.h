#pragma once
#include "ScriptAPI.h"

/// Regression test for deferred Instantiate (REVIEW_NOTES A1).
/// Spawns many cubes every frame, edits them through the returned view before they exist,
/// cancels some in the same frame and destroys the rest after a short lifetime.
/// Attach to any GameObject and enter Play mode; the editor must not crash in Development or Release.
class SpawnStressTest : public ScriptBase {
public:
	EXPOSE_FIELD(int, spawnPerFrame, 50);
	EXPOSE_FIELD(float, testDuration, 5.0f);
	EXPOSE_FIELD(float, cubeLifetime, 1.0f);
	EXPOSE_FIELD(bool, testEnabled, true);

	void Initialize() override;
	void Update(float deltaTime) override;
	void OnDestroy() override;

private:
	static constexpr int kMaxTracked = 8192;

	struct TrackedCube {
		uint32_t id = 0;
		float spawnTime = 0.0f;
	};

	void DestroyExpired(bool destroyAll);

	TrackedCube tracked_[kMaxTracked]{};
	int trackedHead_ = 0;
	int trackedCount_ = 0;
	float elapsed_ = 0.0f;
	int totalSpawned_ = 0;
	int totalCancelled_ = 0;
	int totalDestroyed_ = 0;
	bool finished_ = false;
};
