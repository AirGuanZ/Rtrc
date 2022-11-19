#pragma once

#include <Rtrc/Graphics/Device/Buffer/DynamicBuffer.h>
#include <Rtrc/Math/Angle.h>
#include <Rtrc/Math/Matrix4x4.h>

RTRC_BEGIN

rtrc_struct(CameraConstantBuffer)
{
    rtrc_var(Matrix4x4f, worldToCameraMatrix);
    rtrc_var(Matrix4x4f, cameraToWorldMatrix);
    rtrc_var(Matrix4x4f, cameraToClipMatrix);
    rtrc_var(Matrix4x4f, clipToCameraMatrix);
    rtrc_var(Matrix4x4f, worldToClipMatrix);
    rtrc_var(Matrix4x4f, clipToWorldMatrix);
};

struct PerspectiveProjectionParameters
{
    float fovYRad   = Deg2Rad(50.0f);
    float wOverH    = 1;
    float nearPlane = 0.1f;
    float farPlane  = 100.0f;
};

class RenderCamera
{
public:
    
    const CameraConstantBuffer &GetConstantBufferData() const;

    const Matrix4x4f &GetWorldToCamera() const;
    const Matrix4x4f &GetWorldToClip() const;

private:

    friend class Camera;

    CameraConstantBuffer data_;
};

class Camera
{
public:

    Camera();

    const Vector3f &GetRotation() const;
    const Vector3f &GetPosition() const;
    const PerspectiveProjectionParameters &GetProjection() const;

    const Vector3f &GetLeft() const;
    const Vector3f &GetForward() const;
    const Vector3f &GetUp() const;

    // (0, 0, 0):
    //     forward <- +z
    //     up      <- +y
    //     left    <- +x
    // (rotAlongX, rotAlongY, rotAlongZ):
    //     Z -> X -> Y
    void SetRotation(const Vector3f &rotation);
    void SetPosition(const Vector3f &position);
    void SetProjection(float fovYRad, float wOverH, float nearPlane, float farPlane);
    void CalculateDerivedData();

    void SetUpRenderCamera(RenderCamera &renderCamera) const;

private:

    void SetDirty();
    void ClearDrity();
    void AssertNotDirty() const;

    // Basic members

    Vector3f position_;
    Vector3f rotation_;
    PerspectiveProjectionParameters projParams_;

    // Computed members

    Vector3f forward_;
    Vector3f left_;
    Vector3f up_;

    Matrix4x4f cameraToWorldMatrix_;
    Matrix4x4f worldToCameraMatrix_;
    Matrix4x4f cameraToClipMatrix_;
    Matrix4x4f clipToCameraMatrix_;
    Matrix4x4f worldToClipMatrix_;
    Matrix4x4f clipToWorldMatrix_;

#if RTRC_DEBUG
    bool isDerivedDataDirty_ = true;
#endif
};

inline const CameraConstantBuffer &RenderCamera::GetConstantBufferData() const
{
    return data_;
}

inline Camera::Camera()
{
    CalculateDerivedData();
}

inline const Vector3f &Camera::GetRotation() const
{
    return rotation_;
}

inline const Vector3f &Camera::GetPosition() const
{
    return position_;
}

inline const PerspectiveProjectionParameters &Camera::GetProjection() const
{
    return projParams_;
}

inline const Vector3f &Camera::GetLeft() const
{
    AssertNotDirty();
    return left_;
}

inline const Vector3f &Camera::GetForward() const
{
    AssertNotDirty();
    return forward_;
}

inline const Vector3f &Camera::GetUp() const
{
    AssertNotDirty();
    return up_;
}

inline void Camera::SetRotation(const Vector3f &rotation)
{
    SetDirty();
    rotation_ = rotation;
}

inline void Camera::SetPosition(const Vector3f &position)
{
    SetDirty();
    position_ = position;
}

inline void Camera::SetProjection(float fovYRad, float wOverH, float nearPlane, float farPlane)
{
    SetDirty();
    projParams_ = { fovYRad, wOverH, nearPlane, farPlane };
}

inline void Camera::SetDirty()
{
#if RTRC_DEBUG
    isDerivedDataDirty_ = true;
#endif
}

inline void Camera::ClearDrity()
{
#if RTRC_DEBUG
    isDerivedDataDirty_ = false;
#endif
}

inline void Camera::AssertNotDirty() const
{
#if RTRC_DEBUG
    assert(!isDerivedDataDirty_);
#endif
}

RTRC_END
