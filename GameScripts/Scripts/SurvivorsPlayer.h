#pragma once
#include "SurvivorsShared.h"

// Top-down player: WASD / arrows / left stick to move, aims at the mouse (or right stick),
// fires the "Bullet" prefab automatically. Loses HP when touching an object on LAYER_ENEMY.
class SurvivorsPlayer : public ScriptBase {
public:
	EXPOSE_FIELD(float, moveSpeed, 6.0f);
	EXPOSE_FIELD(int, maxHp, 10);
	EXPOSE_FIELD(float, fireInterval, 0.25f);
	EXPOSE_FIELD(float, invulnerableTime, 0.6f);

	void Initialize() override;
	void Update(float deltaTime) override;
	void OnCollision(GameObject& other) override;

private:
	float fireTimer_ = 0.0f;
	float invulnerableTimer_ = 0.0f;
	Vector3 aim_ = { 0.0f, 0.0f, 1.0f };
};
