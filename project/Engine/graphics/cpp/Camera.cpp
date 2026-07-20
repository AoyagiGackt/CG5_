#include "Camera.h"
#include "WinApp.h"

Camera::Camera()
{
    transform_ = { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -10.0f } };
    fovY_        = 0.45f;
    aspectRatio_ = (float)WinApp::kClientWidth / (float)WinApp::kClientHeight;
    nearClip_    = 0.1f;
    farClip_     = 100.0f;
    viewMatrix_        = MakeIdentity4x4();
    projectionMatrix_  = MakeIdentity4x4();
}

void Camera::RecalcMatrices() const
{
    Matrix4x4 cameraWorldMatrix = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, transform_.rotate, transform_.translate);
    viewMatrix_       = Inverse(cameraWorldMatrix);
    projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_);
    isDirty_ = false;
}
