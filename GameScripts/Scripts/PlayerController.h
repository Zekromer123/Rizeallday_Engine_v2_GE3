#pragma once
#include "base/ScriptAPI.h"

// Note: Fields not marked EXPOSE_FIELD are not preserved across a script hot reload.
class PlayerController : public ScriptBase {
public:
    EXPOSE_FIELD(float, moveSpeed, 6.0f);
    EXPOSE_FIELD(float, jumpForce, 7.5f);
    EXPOSE_FIELD(bool, faceMoveDirection, true);

    void Initialize() override;
    void Update(float deltaTime) override;
    void OnCollision(GameObject& other) override;
    void OnDestroy() override;
};
