#pragma once
#include <RizeEngine.h>

class EditorApp : public Framework {
public:
	EditorApp() = default;
	~EditorApp() override = default;

	void Initialize() override;
	void Update(float deltaTime) override;
	void Draw() override;
	void Finalize() override;

	SceneManager& GetSceneManager() { return sceneManager_; }

	/// Process exit code: result of a `--run-tests` run (0 = passed), always 0 otherwise
	static int GetExitCode();

private:
	SceneManager sceneManager_{};
	bool testMode_ = false;
	int testTransitionCount_ = 0;
	const int kTargetTransitions = 60;
};
