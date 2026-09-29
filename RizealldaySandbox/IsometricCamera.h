#pragma once
#include "3d/Camera.h"
#include <cmath>
#include <algorithm>
class IsometricCamera : public Camera
{
public:
	void Initialize(Camera* camera);
	void Update(const Vector3& targetPosition, float deltaTime);

    // Mengambil sudut Yaw kamera untuk konversi arah gerak WASD
    float GetYaw() const { return yaw_; }
    // Fungsi untuk mengatur jarak zoom kamera
    void SetDistance(float dist) { distance_ = std::clamp(dist, 5.0f, 30.0f); }
    float GetDistance() const { return distance_; }
    //Rotate
    void RotateYaw(float angleDelta);

private:
    Camera* camera_ = nullptr;
    float pitch_ = 0.7854f; // 45 degree in radians
    float yaw_ = 0.7854f; // 45 degree in radians
    float distance_ = 14.0f;
    Vector3 smoothedTarget_ = { 0.0f, 0.0f, 0.0f };
    bool isFirstFrame_ = true;
};

