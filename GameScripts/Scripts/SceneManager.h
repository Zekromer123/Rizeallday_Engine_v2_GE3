#pragma once
#include "base/ScriptAPI.h"

// Note: Fields not marked EXPOSE_FIELD are not preserved across a script hot reload.
class SceneManager : public ScriptBase {
public:

    void Initialize() override;
    void Update(float deltaTime) override;
    void OnDestroy() override;
};
