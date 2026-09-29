#include "IsometricCamera.h"

void IsometricCamera::Initialize(Camera* camera)
{
    camera_ = camera;
    pitch_ = 0.7854f; // ~45 derajat
    yaw_ = 0.7854f; // ~45 derajat
    distance_ = 14.0f;
    isFirstFrame_ = true;
}

void IsometricCamera::Update(const Vector3& targetPosition, float deltaTime)
{
    if (!camera_) return;
    // 1. Pada frame pertama, langsung samakan smoothedTarget agar tidak melompat
    if (isFirstFrame_) {
        smoothedTarget_ = targetPosition;
        isFirstFrame_ = false;
    }
    else {
        // 2. Haluskan posisi target (Lerp hanya pada target pemain yang bergerak)
        float followSpeed = 8.0f;
        float t = std::clamp(followSpeed * deltaTime, 0.0f, 1.0f);
        smoothedTarget_.x += (targetPosition.x - smoothedTarget_.x) * t;
        smoothedTarget_.y += (targetPosition.y - smoothedTarget_.y) * t;
        smoothedTarget_.z += (targetPosition.z - smoothedTarget_.z) * t;
    }
    // 3. Terapkan rotasi kamera secara presisi
    camera_->rotation = { pitch_, yaw_, 0.0f };
    // 4. Hitung posisi orbit kamera langsung dari smoothedTarget_
    Vector3 offset = {
        -distance_ * std::sin(yaw_) * std::cos(pitch_),
         distance_ * std::sin(pitch_),
        -distance_ * std::cos(yaw_) * std::cos(pitch_)
    };
    // 5. Posisi kamera langsung menempel pada titik orbitnya
    camera_->translation = {
        smoothedTarget_.x + offset.x,
        smoothedTarget_.y + offset.y,
        smoothedTarget_.z + offset.z
    };
    // 6. Update matriks kamera
    camera_->Update();
}

void IsometricCamera::RotateYaw(float angleDelta)
{
    yaw_ += angleDelta;

    // Jaga agar sudut tetap di rentang 0 sampai 2*PI (opsional, agar nilai tidak overflow)
    const float twoPi = 6.2831853f;
    if (yaw_ > twoPi) yaw_ -= twoPi;
    if (yaw_ < 0.0f)  yaw_ += twoPi;
}
