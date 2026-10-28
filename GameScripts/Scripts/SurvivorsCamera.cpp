#include "SurvivorsCamera.h"

void SurvivorsCamera::Initialize() {
	if (GameObject* self = GetOwner()) {
		self->translation = { Survivors::gPlayerPos.x + offset.x, Survivors::gPlayerPos.y + offset.y, Survivors::gPlayerPos.z + offset.z };
		self->rotation = { pitch, 0.0f, 0.0f };
		self->rotationQuat = Quaternion::FromEuler(self->rotation);
	}
}

void SurvivorsCamera::Update(float deltaTime) {
	GameObject* self = GetOwner();
	if (!self) return;
	const Vector3 target = { Survivors::gPlayerPos.x + offset.x, Survivors::gPlayerPos.y + offset.y, Survivors::gPlayerPos.z + offset.z };
	// Frame-rate independent smoothing; unscaled time keeps the camera settling while the game is paused
	const float t = 1.0f - std::exp(-followSharpness * GetUnscaledDeltaTime());
	(void)deltaTime;
	self->translation.x += (target.x - self->translation.x) * t;
	self->translation.y += (target.y - self->translation.y) * t;
	self->translation.z += (target.z - self->translation.z) * t;
	self->rotation = { pitch, 0.0f, 0.0f };
	self->rotationQuat = Quaternion::FromEuler(self->rotation);
}

REGISTER_SCRIPT(SurvivorsCamera)
