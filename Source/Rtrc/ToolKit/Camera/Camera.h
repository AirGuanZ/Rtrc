#pragma once

#include <Rtrc/Core/Math/Angle.h>
#include <Rtrc/Core/Math/Matrix4x4.h>

RTRC_BEGIN

class Camera
{
public:

    void SetPosition(const Vector3f &position) { position_ = position; }

    // (0, 0, 0):
    //     forward <- +z
    //     up      <- +y
    //     left    <- +x
    // (rotAlongX, rotAlongY, rotAlongZ):
    //     Z -> X -> Y
    void SetRotation(const Vector3f &rotation) { rotation_ = rotation; }

    void SetLookAt(const Vector3f &position, const Vector3f &up, const Vector3f &target);

    void SetOrtho      (bool orthoMode)    { ortho_       = orthoMode;       }
    void SetAspectRatio(float aspectRatio) { aspectRatio_ = aspectRatio;     }
    void SetFovYRad    (float fovRad)      { fovYRad_     = fovRad;          }
    void SetFovYDeg    (float fovDeg)      { fovYRad_     = Deg2Rad(fovDeg); }
    void SetNear       (float _near)       { near_       = _near;            }
    void SetFar        (float _far)        { far_        = _far;             }
    void SetOrthoWidth (float orthoWidth)  { orthoWidth_  = orthoWidth;      }

    const Vector3f &GetPosition() const { return position_; }
    const Vector3f &GetRotation() const { return rotation_; }

    bool  GetOrtho()       const { return ortho_; }
    float GetAspectRatio() const { return aspectRatio_; }
    float GetFovYRad()     const { return fovYRad_; }
    float GetNear()        const { return near_; }
    float GetFar()         const { return far_; }
    float GetOrthoWidth()  const { return orthoWidth_; }

    void UpdateDerivedData();

    const Vector3f &GetFront() const { return front_; }
    const Vector3f &GetLeft()  const { return left_;    }
    const Vector3f &GetUp()    const { return up_;      }

    const Matrix4x4f &GetWorldToCamera() const { return worldToCamera_; }
    const Matrix4x4f &GetCameraToWorld() const { return cameraToWorld_; }
    const Matrix4x4f &GetWorldToClip()   const { return worldToClip_;   }
    const Matrix4x4f &GetClipToWorld()   const { return clipToWorld_;   }
    const Matrix4x4f &GetCameraToClip()  const { return cameraToClip_;  }
    const Matrix4x4f &GetClipToCamera()  const { return clipToCamera_;  }

    const Vector3f *GetCameraRays() const { return cameraRays_; }
    const Vector3f *GetWorldRays()  const { return worldRays_;  }

private:

    // Basic

    Vector3f position_;
    Vector3f rotation_;

    bool  ortho_       = true;
    float aspectRatio_ = 1;
    float fovYRad_     = Deg2Rad(60);
    float near_        = 0.1f;
    float far_         = 1000.0f;
    float orthoWidth_  = 1;

    // Derived

    Vector3f front_;
    Vector3f left_;
    Vector3f up_;

    Matrix4x4f worldToCamera_;
    Matrix4x4f cameraToWorld_;
    Matrix4x4f worldToClip_;
    Matrix4x4f clipToWorld_;
    Matrix4x4f cameraToClip_;
    Matrix4x4f clipToCamera_;

    Vector3f cameraRays_[4];
    Vector3f worldRays_[4];
};

RTRC_END
