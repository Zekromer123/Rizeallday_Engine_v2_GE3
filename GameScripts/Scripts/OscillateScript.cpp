#include "OscillateScript.h"
#include <cmath>
#include <cstdio>

void OscillateScript::Initialize() {
	phase = 0.0f;
	if (GameObject* self = GetOwner()) {
		baseY = self->translation.y;
		baseInitialized = true;
		char msg[256]{};
		std::snprintf(msg, sizeof(msg), "[OscillateScript] Initialized on '%s' (baseY=%.2f)", self->name, baseY);
		Log(msg, 0);
	}
}

void OscillateScript::Update(float deltaTime) {
	phase += deltaTime * frequency;
	if (GameObject* self = GetOwner()) {
		if (!baseInitialized) {
			baseY = self->translation.y;
			baseInitialized = true;
		}
		self->translation.y = baseY + std::sin(phase) * amplitude;
		self->dirtyFromScript = true;
	}
}

void OscillateScript::OnCollision(GameObject& other) {
	if (GameObject* self = GetOwner()) {
		char msg[256]{};
		std::snprintf(msg, sizeof(msg), "[OscillateScript] '%s' collided with '%s'", self->name, other.name);
		Log(msg, 0);
	}
}

void OscillateScript::OnDestroy() {
	if (GameObject* self = GetOwner()) {
		char msg[256]{};
		std::snprintf(msg, sizeof(msg), "[OscillateScript] OnDestroy on '%s'", self->name);
		Log(msg, 0);
	}
}

void OscillateScript::OnAfterReload() {
	if (GameObject* self = GetOwner()) {
		char msg[256]{};
		std::snprintf(msg, sizeof(msg), "[OscillateScript] Hot-reloaded on '%s' (amplitude=%.2f, frequency=%.2f)", self->name, amplitude, frequency);
		Log(msg, 0);
	}
}

REGISTER_SCRIPT(OscillateScript)
