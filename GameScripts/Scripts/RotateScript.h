#pragma once
#include "ScriptAPI.h"

/// Rotates the owner GameObject continuously around a configurable axis.
/// Note: Fields marked with EXPOSE_FIELD are shown in the Inspector and preserved across hot reloads and scene saves.
/// Fields NOT marked with EXPOSE_FIELD are reset to their default values when GameScripts.dll is hot-reloaded.
class RotateScript : public ScriptBase {
public:
	EXPOSE_FIELD(float, rotationSpeed, 1.5f);
	EXPOSE_FIELD(Vector3, rotationAxis, Vector3{ 1.0f, 2.0f, 1.0f });
	EXPOSE_FIELD(bool, activeRotation, true);

	void Initialize() override;
	void Update(float deltaTime) override;
	void OnCollision(GameObject& other) override;
	void OnDestroy() override;
	void OnAfterReload() override;
};
