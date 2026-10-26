#pragma once
#include <RizeEngine.h>
#include "2d/Sprite.h"
#include <memory>

/**
 * @brief TitleScene renders the 3.2-second Rizeallday Engine startup splash animation
 *        and transitions automatically into EditorScene.
 *
 * NOTE ON SCENE TERMINOLOGY:
 * TitleScene is a top-level application state managed by SceneManager (State Pattern).
 * This is distinct from editor scene files (*.scene.json), which are serialized GameObject
 * scenes loaded, saved, and manipulated within EditorSuite.
 */
class TitleScene : public BaseScene {
public:
	TitleScene() = default;
	~TitleScene() override;

	void Initialize() override;
	void Finalize() override;
	void Update(float deltaTime) override;
	void Draw() override;

private:
	std::unique_ptr<Sprite> bgSprite_;
	std::unique_ptr<Sprite> iconSprite_;
	std::unique_ptr<Sprite> textSprite_;
	std::unique_ptr<Sprite> lineSprite_;

	float elapsed_ = 0.0f;
	bool transitioned_ = false;
};
