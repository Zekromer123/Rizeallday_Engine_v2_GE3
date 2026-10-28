#include "SurvivorsEnemy.h"

void SurvivorsEnemy::Update(float deltaTime) {
	GameObject* self = GetOwner();
	if (!self || Survivors::gGameOver) return;

	const float dx = Survivors::gPlayerPos.x - self->translation.x;
	const float dz = Survivors::gPlayerPos.z - self->translation.z;
	const float dist = std::sqrt(dx * dx + dz * dz);
	if (dist > 0.05f) {
		const float step = speed * Survivors::gEnemySpeedScale * deltaTime;
		self->translation.x += dx / dist * step;
		self->translation.z += dz / dist * step;
		Survivors::SetYaw(self, Survivors::YawFromDirection(dx, dz));
	}
}

void SurvivorsEnemy::OnCollision(GameObject& other) {
	if (other.layer != LAYER_PLAYER_BULLET || hp_ <= 0) return;
	hp_ -= 1;
	if (hp_ <= 0) {
		Survivors::gKills += 1;
		DespawnSelf();
	}
}

REGISTER_SCRIPT(SurvivorsEnemy)
