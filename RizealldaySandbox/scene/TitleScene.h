#pragma once
#include <RizeEngine.h>

/**
 * @brief TitleScene is a demo application state.
 *
 * NOTE ON SCENE TERMINOLOGY:
 * TitleScene is a top-level application state managed by SceneManager (State Pattern).
 * This is distinct from editor scene files (*.scene.json), which are serialized GameObject
 * scenes loaded, saved, and manipulated within EditorSuite.
 */
class TitleScene : public BaseScene {
public:
	TitleScene() = default;
	~TitleScene() override = default;

	void Initialize() override;
	void Finalize() override;
	void Update(float deltaTime) override;
	void Draw() override;
};
