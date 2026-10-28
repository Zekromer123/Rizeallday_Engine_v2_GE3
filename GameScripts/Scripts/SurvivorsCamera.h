#pragma once
#include "SurvivorsShared.h"

// Put on the Main Camera: follows the player from above at a fixed angle.
class SurvivorsCamera : public ScriptBase {
public:
	EXPOSE_FIELD(Vector3, offset, Vector3{ 0.0f, 18.0f, -10.0f });
	EXPOSE_FIELD(float, pitch, 1.05f);       // radians, looking down
	EXPOSE_FIELD(float, followSharpness, 8.0f);

	void Initialize() override;
	void Update(float deltaTime) override;
};
