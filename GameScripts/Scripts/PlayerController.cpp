#include "PlayerController.h"
#include <dinput.h>

void PlayerController::Initialize() {
    if (GameObject* self = GetOwner()) {
        (void)self;
    }
}

void PlayerController::Update(float deltaTime) {
    GameObject* self = GetOwner();
    if (!self)
    {
        return;
    }
    Vector3 moveDir = { 0.0f, 0.0f, 0.0f };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
        moveDir.z += 1.0f;
       
    } if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
        moveDir.z -= 1.0f;
       
    } if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
        moveDir.x += 1.0f;
       
    }if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
        moveDir.x -= 1.0f;
    }

    //Animation
    const bool isMoving = (moveDir.x != 0.0f || moveDir.z != 0.0f);
    SetAnimBool("isRunning",isMoving);

   //Normalize
    float length = std::sqrt(moveDir.x * moveDir.x + moveDir.z * moveDir.z);
    if (length > 0.0001f) {
        moveDir.x /= length;
        moveDir.z /= length;
        // Player
        self->translation.x += moveDir.x * moveSpeed * deltaTime;
        self->translation.z += moveDir.z * moveSpeed * deltaTime;
        // Rotate Player
        if (faceMoveDirection) {
            self->rotation.y = std::atan2(moveDir.x, moveDir.z);
            self->rotationQuat = Quaternion::FromEuler(self->rotation);
        }
    }
    // Jump (if player has rigidbody)
    if (IsKeyTriggered(DIK_SPACE) && self->isGrounded) {
        self->velocity.y = jumpForce;
        self->isGrounded = false;
    }
   
}

void PlayerController::OnCollision(GameObject& other) {
    (void)other;
}

void PlayerController::OnDestroy() {
}

REGISTER_SCRIPT(PlayerController)
