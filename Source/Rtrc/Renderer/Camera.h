#pragma once

#include <Rtrc/Graphics/Device/Buffer/DynamicBuffer.h>
#include <Rtrc/Math/Angle.h>
#include <Rtrc/Math/Matrix4x4.h>

RTRC_BEGIN

rtrc_struct(CameraConstantBuffer)
{
    rtrc_var(Matrix4x4f,  worldToCameraMatrix);
    rtrc_var(Matrix4x4f,  cameraToWorldMatrix);
    rtrc_var(Matrix4x4f,  cameraToClipMatrix);
    rtrc_var(Matrix4x4f,  clipToCameraMatrix);
    rtrc_var(Matrix4x4f,  worldToClipMatrix);
    rtrc_var(Matrix4x4f,  clipToWorldMatrix);
    rtrc_var(Vector3f[4], cameraRays);
    rtrc_var(Vector3f[4], worldRays);
};

struct PerspectiveProjectionParameters
{
    float fovYRad   = Deg2Rad(50.0f);
    float wOverH    = 1;
    float nearPlane = 0.1f;
    float farPlane  = 100.0f;
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

    const CameraConstantBuffer &GetConstantBufferData() const;
    const Matrix4x4f &GetWorldToCameraMatrix() const;
    const Matrix4x4f &GetCameraToWorldMatrix() const;
    const Matrix4x4f &GetCameraToClipMatrix() const;
    const Matrix4x4f &GetClipToCameraMatrix() const;
    const Matrix4x4f &GetWorldToClipMatrix() const;
    const Matrix4x4f &GetClipToWorldMatrix() const;

private:

    void SetDirty();
    void ClearDrity();
    void AssertNotDirty() const;

    // Basic data

    Vector3f position_;
    Vector3f rotation_;
    PerspectiveProjectionParameters projParams_;

    // Derived data

    Vector3f forward_;
    Vector3f left_;
    Vector3f up_;

    CameraConstantBuffer constantBufferData_;

#if RTRC_DEBUG
    bool isDerivedDataDirty_ = true;
#endif
};

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

inline const CameraConstantBuffer &Camera::GetConstantBufferData() const
{
    return constantBufferData_;
}

inline const Matrix4x4f &Camera::GetWorldToCameraMatrix() const
{
    return constantBufferData_.worldToCameraMatrix;
}

inline const Matrix4x4f &Camera::GetCameraToWorldMatrix() const
{
    return constantBufferData_.cameraToWorldMatrix;
}

inline const Matrix4x4f &Camera::GetCameraToClipMatrix() const
{
    return constantBufferData_.cameraToClipMatrix;
}

inline const Matrix4x4f &Camera::GetClipToCameraMatrix() const
{
    return constantBufferData_.clipToCameraMatrix;
}

inline const Matrix4x4f &Camera::GetWorldToClipMatrix() const
{
    return constantBufferData_.worldToClipMatrix;
}

inline const Matrix4x4f &Camera::GetClipToWorldMatrix() const
{
    return constantBufferData_.clipToWorldMatrix;
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
