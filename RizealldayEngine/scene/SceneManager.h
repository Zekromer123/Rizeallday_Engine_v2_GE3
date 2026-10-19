#pragma once
#include "scene/BaseScene.h"
#include <memory>

/**
 * @brief SceneManager manages application-level scene transitions using the State Pattern.
 *
 * Transitions requested via ChangeScene() are deferred until the beginning of Update(),
 * ensuring safe teardown and initialization without disrupting rendering or UI event processing.
 */
class SceneManager {
public:
	SceneManager() = default;
	~SceneManager();

	// Non-copyable, non-movable
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	void ChangeScene(std::unique_ptr<BaseScene> nextScene);
	void Update(float deltaTime);
	void Draw();
	void Finalize();

	BaseScene* GetCurrentScene() const { return currentScene_.get(); }
	bool HasPendingScene() const { return nextScene_ != nullptr; }

private:
	std::unique_ptr<BaseScene> currentScene_;
	std::unique_ptr<BaseScene> nextScene_;
};
