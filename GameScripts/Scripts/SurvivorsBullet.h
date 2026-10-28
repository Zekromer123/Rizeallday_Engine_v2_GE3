#pragma once
#include "SurvivorsShared.h"

// Flies forward (+Z of its rotation) and returns to the pool after `lifetime` or on hitting an enemy.
class SurvivorsBullet : public ScriptBase {
public:
	EXPOSE_FIELD(float, speed, 18.0f);
	EXPOSE_FIELD(float, lifetime, 1.2f);

	void Initialize() override { age_ = 0.0f; } // runs again every time the pool reuses this bullet
	void Update(float deltaTime) override;
	void OnCollision(GameObject& other) override;

private:
	float age_ = 0.0f;
};
