#pragma once

#include <Core/Math/Angle.h>
#include <Core/Math/Matrix4x4.h>
#include <Core/SignalSlot.h>

RTRC_BEGIN

rtrc_struct(CameraData)
{
    float3 position;
    float3 rotation;
    float3 front;
    float3 left;
    float3 up;

    float4x4 worldToCamera;
    float4x4 cameraToWorld;

    float4x4 worldToClip;
    float4x4 clipToWorld;

    float4x4 cameraToClip;
    float4x4 clipToCamera;

    float3 cameraRays[4];
    float3 worldRays[4];

    float fovYRad   = Deg2Rad(50);
    float wOverH    = 1;
    float nearPlane = 0.1f;
    float farPlane  = 100.0f;
};

class Camera : public WithUniqueObjectID, CameraData
{
public:

    Camera();

    const Vector3f &GetRotation() const;
    const Vector3f &GetPosition() const;

    float GetFOVYRad() const;
    float GetWOverH() const;
    float GetNearPlane() const;
    float GetFarPlane() const;
    
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

    const CameraData &GetData() const;

    template<typename...Args>
    Connection OnDestroy(Args&&...args) const { return onDestroy_.Connect(std::forward<Args>(args)...); }

private:

    mutable Signal<SignalThreadPolicy::SpinLock> onDestroy_;
};

inline Camera::Camera()
{
    CalculateDerivedData();
}

inline const Vector3f &Camera::GetRotation() const
{
    return rotation;
}

inline const Vector3f &Camera::GetPosition() const
{
    return position;
}

inline float Camera::GetFOVYRad() const
{
    return fovYRad;
}

inline float Camera::GetWOverH() const
{
    return wOverH;
}

inline float Camera::GetNearPlane() const
{
    return nearPlane;
}

inline float Camera::GetFarPlane() const
{
    return farPlane;
}

inline const Vector3f &Camera::GetLeft() const
{
    return left;
}

inline const Vector3f &Camera::GetForward() const
{
    return front;
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

inline void Camera::SetProjection(float _fovYRad, float _wOverH, float _nearPlane, float _farPlane)
{
    fovYRad = _fovYRad;
    wOverH = _wOverH;
    nearPlane = _nearPlane;
    farPlane = _farPlane;
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

inline const CameraData &Camera::GetData() const
{
    return *this;
}

RTRC_END
