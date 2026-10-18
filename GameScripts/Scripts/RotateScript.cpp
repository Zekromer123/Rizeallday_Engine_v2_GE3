#include "RotateScript.h"
#include <cstdio>

void RotateScript::Initialize() {
	if (GameObject* self = GetOwner()) {
		char msg[256]{};
		std::snprintf(msg, sizeof(msg), "[RotateScript] Initialized on '%s'", self->name);
		Log(msg, 0);
	}
}

void RotateScript::Update(float deltaTime) {
	if (!activeRotation) return;
	if (GameObject* self = GetOwner()) {
		self->rotation.x += rotationAxis.x * rotationSpeed * deltaTime;
		self->rotation.y += rotationAxis.y * rotationSpeed * deltaTime;
		self->rotation.z += rotationAxis.z * rotationSpeed * deltaTime;
		self->rotationQuat = Quaternion::FromEuler(self->rotation);
		self->dirtyFromScript = true;
	}
}

void RotateScript::OnCollision(GameObject& other) {
	if (GameObject* self = GetOwner()) {
		char msg[256]{};
		std::snprintf(msg, sizeof(msg), "[RotateScript] '%s' collided with '%s'", self->name, other.name);
		Log(msg, 0);
	}
}

void RotateScript::OnDestroy() {
	if (GameObject* self = GetOwner()) {
		char msg[256]{};
		std::snprintf(msg, sizeof(msg), "[RotateScript] OnDestroy on '%s'", self->name);
		Log(msg, 0);
	}
}

void RotateScript::OnAfterReload() {
	if (GameObject* self = GetOwner()) {
		char msg[256]{};
		std::snprintf(msg, sizeof(msg), "[RotateScript] Hot-reloaded on '%s' (rotationSpeed=%.2f)", self->name, rotationSpeed);
		Log(msg, 0);
	}
}

REGISTER_SCRIPT(RotateScript)
