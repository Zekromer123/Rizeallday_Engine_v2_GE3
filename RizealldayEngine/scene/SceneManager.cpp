#include "scene/SceneManager.h"

SceneManager::~SceneManager() {
	Finalize();
}

void SceneManager::ChangeScene(std::unique_ptr<BaseScene> nextScene) {
	nextScene_ = std::move(nextScene);
}

void SceneManager::Update(float deltaTime) {
	// Transitions only happen at this safe point, never inside Draw or a UI callback
	if (nextScene_) {
		if (currentScene_) {
			currentScene_->Finalize();
		}
		currentScene_ = std::move(nextScene_);
		currentScene_->SetSceneManager(this);
		currentScene_->Initialize();
	}

	if (currentScene_) {
		currentScene_->Update(deltaTime);
	}
}

void SceneManager::Draw() {
	if (currentScene_) {
		currentScene_->Draw();
	}
}

void SceneManager::Finalize() {
	if (currentScene_) {
		currentScene_->Finalize();
		currentScene_.reset();
	}
	nextScene_.reset();
}
