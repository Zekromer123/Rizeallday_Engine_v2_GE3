#include "3d/Camera.h"
#include "math/MathUtil.h"

void Camera::Initialize() {
    matView_ = Mathematics::MakeIdentity4x4();
    matProjection_ = Mathematics::MakePerspectiveFovMatrix(fovY, aspectRatio, nearClip, farClip);
    matViewProjection_ = Mathematics::Multiply(matView_, matProjection_);
}

void Camera::Update() {
    Matrix4x4 cameraWorldMatrix = Mathematics::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, rotation, translation);
    matView_ = Mathematics::Inverse(cameraWorldMatrix);
    matProjection_ = Mathematics::MakePerspectiveFovMatrix(fovY, aspectRatio, nearClip, farClip);
    matViewProjection_ = Mathematics::Multiply(matView_, matProjection_);
}
