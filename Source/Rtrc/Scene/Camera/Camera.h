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

class Camera : public WithUniqueObjectID
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

private:

    // Basic data

    Vector3f position_;
    Vector3f rotation_;
    PerspectiveProjectionParameters projParams_;

    // Derived data

    Vector3f forward_;
    Vector3f left_;
    Vector3f up_;

    Matrix4x4f worldToCamera_;
    Matrix4x4f cameraToWorld_;
    Matrix4x4f cameraToClip_;
    Matrix4x4f clipToCamera_;
    Matrix4x4f worldToClip_;
    Matrix4x4f clipToWorld_;

    Vector3f cameraRays_[4];
    Vector3f worldRays_[4];
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
    return left_;
}

inline const Vector3f &Camera::GetForward() const
{
    return forward_;
}

inline const Vector3f &Camera::GetUp() const
{
    return up_;
}

inline void Camera::SetRotation(const Vector3f &rotation)
{
    rotation_ = rotation;
}

inline void Camera::SetPosition(const Vector3f &position)
{
    position_ = position;
}

inline void Camera::SetProjection(float fovYRad, float wOverH, float nearPlane, float farPlane)
{
    projParams_ = { fovYRad, wOverH, nearPlane, farPlane };
}

inline const Matrix4x4f &Camera::GetWorldToCameraMatrix() const
{
    return worldToCamera_;
}

inline const Matrix4x4f &Camera::GetCameraToWorldMatrix() const
{
    return cameraToWorld_;
}

inline const Matrix4x4f &Camera::GetCameraToClipMatrix() const
{
    return cameraToClip_;
}

inline const Matrix4x4f &Camera::GetClipToCameraMatrix() const
{
    return clipToCamera_;
}

inline const Matrix4x4f &Camera::GetWorldToClipMatrix() const
{
    return worldToClip_;
}

inline const Matrix4x4f &Camera::GetClipToWorldMatrix() const
{
    return clipToWorld_;
}

inline const Camera::CornerRays &Camera::GetWorldRays() const
{
    return worldRays_;
}

inline const Camera::CornerRays &Camera::GetCameraRays() const
{
    return cameraRays_;
}

RTRC_END
