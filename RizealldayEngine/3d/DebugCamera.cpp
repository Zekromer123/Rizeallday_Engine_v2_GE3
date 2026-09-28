#include "3d/DebugCamera.h"

void DebugCamera::Initialize()
{
    matRot_ = Mathematics::MakeIdentity4x4();
    matProjection_ = Mathematics::MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);

    
}


void DebugCamera::Update(Input* input)
{

    if (input->PushMouseButton(1) && !input->PushKey(DIK_LSHIFT))
    {
        float mouseRotateSpeed = 0.005f;
        float dx = input->GetMouseY() * mouseRotateSpeed; // Pitch (Rotasi X)
        float dy = input->GetMouseX() * mouseRotateSpeed; // Yaw (Rotasi Y)
        Matrix4x4 matRotDelta = Mathematics::MakeIdentity4x4();
        if (dx != 0.0f) matRotDelta = Mathematics::Multiply(matRotDelta, Mathematics::MakeRotateXMatrix(dx));
        if (dy != 0.0f) matRotDelta = Mathematics::Multiply(matRotDelta, Mathematics::MakeRotateYMatrix(dy));
        matRot_ = Mathematics::Multiply(matRotDelta, matRot_);
    }

// Movement
    float moveSpeed = 0.05f;
    Vector3 move = { 0.0f, 0.0f, 0.0f };
    // Keyboard controls
    if (input->PushKey(DIK_W)) move.z += moveSpeed;
    if (input->PushKey(DIK_S)) move.z -= moveSpeed;
    if (input->PushKey(DIK_D)) move.x += moveSpeed;
    if (input->PushKey(DIK_A)) move.x -= moveSpeed;
    if (input->PushKey(DIK_E)) move.y += moveSpeed;
    if (input->PushKey(DIK_Q)) move.y -= moveSpeed;
    
    // Panning with Shift + Right Click
    if (input->PushMouseButton(1) && input->PushKey(DIK_LSHIFT))
    {
        float mousePanSpeed = 0.05f;
        move.x -= input->GetMouseX() * mousePanSpeed;
        move.y += input->GetMouseY() * mousePanSpeed;
    }
    // Panning with middle mouse button
    else if (input->PushMouseButton(2))
    {
        float mousePanSpeed = 0.05f;
        move.x -= input->GetMouseX() * mousePanSpeed;
        move.y += input->GetMouseY() * mousePanSpeed;
    }

    // Local movement to camera view direction
    Vector3 rotatedMove = Mathematics::Transform(move, matRot_);
    // add Camera position with rotated movement
    translation_.x += rotatedMove.x;
    translation_.y += rotatedMove.y;
    translation_.z += rotatedMove.z;
    // Update View Matrix
    Matrix4x4 matTranslate = Mathematics::MakeTranslateMatrix(translation_);
    Matrix4x4 cameraWorldMatrix = Mathematics::Multiply(matRot_, matTranslate);
    matView_ = Mathematics::Inverse(cameraWorldMatrix);
}
