#include "SurvivorsPlayer.h"

void SurvivorsPlayer::Initialize() {
	Survivors::gPlayerMaxHp = maxHp > 0 ? maxHp : 1;
	Survivors::gPlayerHp = Survivors::gPlayerMaxHp;
	Survivors::gGameOver = false;
	fireTimer_ = 0.0f;
	invulnerableTimer_ = 0.0f;
	if (GameObject* self = GetOwner()) {
		Survivors::gPlayerPos = self->translation;
	}
}

void SurvivorsPlayer::Update(float deltaTime) {
	GameObject* self = GetOwner();
	if (!self || Survivors::gGameOver) return;

	// Movement on the XZ plane (GetMoveAxis already combines keys + stick and limits the length to 1)
	const Vector2 move = GetMoveAxis();
	self->translation.x += move.x * moveSpeed * deltaTime;
	self->translation.z += move.y * moveSpeed * deltaTime;

	// Aim: right stick first, then the mouse, otherwise keep the last direction
	const Vector2 stick = GetRightStick();
	Vector3 mouseWorld{};
	if (stick.x * stick.x + stick.y * stick.y > 0.09f) {
		aim_ = { stick.x, 0.0f, stick.y };
	} else if (GetMouseWorldPosition(mouseWorld, self->translation.y)) {
		const float dx = mouseWorld.x - self->translation.x;
		const float dz = mouseWorld.z - self->translation.z;
		if (dx * dx + dz * dz > 0.01f) aim_ = { dx, 0.0f, dz };
	} else if (move.x != 0.0f || move.y != 0.0f) {
		aim_ = { move.x, 0.0f, move.y };
	}
	const float len = std::sqrt(aim_.x * aim_.x + aim_.z * aim_.z);
	if (len > 0.0001f) aim_ = { aim_.x / len, 0.0f, aim_.z / len };
	const float yaw = Survivors::YawFromDirection(aim_.x, aim_.z);
	Survivors::SetYaw(self, yaw);

	// Auto fire
	fireTimer_ -= deltaTime;
	if (fireTimer_ <= 0.0f) {
		fireTimer_ = fireInterval;
		const Vector3 spawnPos = { self->translation.x + aim_.x * 0.8f, self->translation.y, self->translation.z + aim_.z * 0.8f };
		if (GameObject* bullet = InstantiatePrefab("Bullet", spawnPos)) {
			Survivors::SetYaw(bullet, yaw); // the bullet flies along its own forward (+Z) axis
		}
	}

	if (invulnerableTimer_ > 0.0f) invulnerableTimer_ -= deltaTime;
	Survivors::gPlayerPos = self->translation;
}

void SurvivorsPlayer::OnCollision(GameObject& other) {
	if (other.layer != LAYER_ENEMY || invulnerableTimer_ > 0.0f || Survivors::gGameOver) return;
	invulnerableTimer_ = invulnerableTime;
	Survivors::gPlayerHp -= 1;
	if (Survivors::gPlayerHp <= 0) {
		Survivors::gPlayerHp = 0;
		Survivors::gGameOver = true;
	}
}

REGISTER_SCRIPT(SurvivorsPlayer)
