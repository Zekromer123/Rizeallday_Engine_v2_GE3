#include "SurvivorsBullet.h"

void SurvivorsBullet::Update(float deltaTime) {
	GameObject* self = GetOwner();
	if (!self) return;
	const float yaw = self->rotation.y;
	self->translation.x += std::sin(yaw) * speed * deltaTime;
	self->translation.z += std::cos(yaw) * speed * deltaTime;

	age_ += deltaTime;
	if (age_ >= lifetime) {
		DespawnSelf();
	}
}

void SurvivorsBullet::OnCollision(GameObject& other) {
	if (other.layer == LAYER_ENEMY) {
		DespawnSelf();
	}
}

REGISTER_SCRIPT(SurvivorsBullet)
