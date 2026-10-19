#pragma once

class SceneManager;

/**
 * @brief BaseScene represents an application state (State Pattern).
 *
 * NOTE ON SCENE TERMINOLOGY:
 * These C++ BaseScene subclasses (e.g. TitleScene, EditorScene) represent distinct
 * top-level application states managed by SceneManager.
 * They are completely separate from the editor's GameObject scene files (*.scene.json),
 * which represent serialized GameObject hierarchies edited and saved within EditorSuite.
 */
class BaseScene {
public:
	virtual ~BaseScene() = default;

	virtual void Initialize() = 0;
	virtual void Finalize() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Draw() = 0;

	void SetSceneManager(SceneManager* sceneManager) { sceneManager_ = sceneManager; }

protected:
	SceneManager* sceneManager_ = nullptr;
};
