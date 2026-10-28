#pragma once
#include "SurvivorsShared.h"

// Walks towards the player; dies after `maxHp` bullet hits (returns to the "Enemy" pool).
class SurvivorsEnemy : public ScriptBase {
public:
	EXPOSE_FIELD(float, speed, 2.5f);
	EXPOSE_FIELD(int, maxHp, 3);

	void Initialize() override { hp_ = maxHp; }
	void Update(float deltaTime) override;
	void OnCollision(GameObject& other) override;

private:
	int hp_ = 1;
};
