#pragma once
#include "ScriptAPI.h"

/// Sinusoidally oscillates the owner GameObject along the Y axis.
/// Note: Fields marked with EXPOSE_FIELD are shown in the Inspector and preserved across hot reloads and scene saves.
/// Fields NOT marked with EXPOSE_FIELD (such as internal timers if not exposed) are not preserved across a reload.
class OscillateScript : public ScriptBase {
public:
	EXPOSE_FIELD(float, amplitude, 0.5f);
	EXPOSE_FIELD(float, frequency, 3.0f);
	EXPOSE_FIELD(float, phase, 0.0f);
	EXPOSE_FIELD(float, baseY, 0.0f);
	EXPOSE_FIELD(bool, baseInitialized, false);
	EXPOSE_FIELD(char[64], tagNote, "Oscillator");

	void Initialize() override;
	void Update(float deltaTime) override;
	void OnCollision(GameObject& other) override;
	void OnDestroy() override;
	void OnAfterReload() override;
};
