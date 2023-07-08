#pragma once

#include <Rtrc/Math/Angle.h>
#include <Rtrc/Math/Matrix4x4.h>

RTRC_BEGIN

struct PerspectiveProjectionParameters
{
    float fovYRad   = Deg2Rad(50.0f);
    float wOverH    = 1;
    float nearPlane = 0.1f;
    float farPlane  = 100.0f;
};

struct CameraRenderData
{
    Vector3f position;
    Vector3f rotation;

    PerspectiveProjectionParameters projParams;

    Vector3f forward;
    Vector3f left;
    Vector3f up;

    Matrix4x4f worldToCamera;
    Matrix4x4f cameraToWorld;
    Matrix4x4f cameraToClip;
    Matrix4x4f clipToCamera;
    Matrix4x4f worldToClip;
    Matrix4x4f clipToWorld;

    Vector3f cameraRays[4];
    Vector3f worldRays[4];

    UniqueId originalId;
};

class Camera : public WithUniqueObjectID, CameraRenderData
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
    void SetLookAt(const Vector3f &up, const Vector3f &destination);
    void SetPosition(const Vector3f &position);
    void SetProjection(float fovYRad, float wOverH, float nearPlane, float farPlane);
    void CalculateDerivedData();

    const Matrix4x4f &GetWorldToCameraMatrix() const;
    const Matrix4x4f &GetCameraToWorldMatrix() const;
    const Matrix4x4f &GetCameraToClipMatrix() const;
    const Matrix4x4f &GetClipToCameraMatrix() const;
    const Matrix4x4f &GetWorldToClipMatrix() const;
    const Matrix4x4f &GetClipToWorldMatrix() const;

    using CornerRays = Vector3f[4];
    const CornerRays &GetWorldRays() const;
    const CornerRays &GetCameraRays() const;

    const CameraRenderData &GetRenderCamera() const;
};

inline Camera::Camera()
{
    CalculateDerivedData();
    originalId = GetUniqueID();
}

inline const Vector3f &Camera::GetRotation() const
{
    return rotation;
}

inline const Vector3f &Camera::GetPosition() const
{
    return position;
}

inline const PerspectiveProjectionParameters &Camera::GetProjection() const
{
    return projParams;
}

inline const Vector3f &Camera::GetLeft() const
{
    return left;
}

inline const Vector3f &Camera::GetForward() const
{
    return forward;
}

inline const Vector3f &Camera::GetUp() const
{
    return up;
}

inline void Camera::SetRotation(const Vector3f &_rotation)
{
    rotation = _rotation;
}

inline void Camera::SetPosition(const Vector3f &_position)
{
    position = _position;
}

inline void Camera::SetProjection(float fovYRad, float wOverH, float nearPlane, float farPlane)
{
    projParams = { fovYRad, wOverH, nearPlane, farPlane };
}

inline const Matrix4x4f &Camera::GetWorldToCameraMatrix() const
{
    return worldToCamera;
}

inline const Matrix4x4f &Camera::GetCameraToWorldMatrix() const
{
    return cameraToWorld;
}

inline const Matrix4x4f &Camera::GetCameraToClipMatrix() const
{
    return cameraToClip;
}

inline const Matrix4x4f &Camera::GetClipToCameraMatrix() const
{
    return clipToCamera;
}

inline const Matrix4x4f &Camera::GetWorldToClipMatrix() const
{
    return worldToClip;
}

inline const Matrix4x4f &Camera::GetClipToWorldMatrix() const
{
    return clipToWorld;
}

inline const Camera::CornerRays &Camera::GetWorldRays() const
{
    return worldRays;
}

inline const Camera::CornerRays &Camera::GetCameraRays() const
{
    return cameraRays;
}

inline const CameraRenderData &Camera::GetRenderCamera() const
{
    return *this;
}

RTRC_END
