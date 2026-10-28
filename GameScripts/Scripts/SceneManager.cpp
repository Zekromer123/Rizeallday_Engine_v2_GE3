#include "SceneManager.h"

void SceneManager::Initialize() {
    if (GameObject* self = GetOwner()) {
        (void)self;
    }
}

void SceneManager::Update(float deltaTime) {
    if (GameObject* self = GetOwner()) {
        if (IsKeyTriggered(KEY_SPACE))
        {
            LoadScene("SurvivorsDemo");
        }
    }
}

void SceneManager::OnDestroy() {
}

REGISTER_SCRIPT(SceneManager)
