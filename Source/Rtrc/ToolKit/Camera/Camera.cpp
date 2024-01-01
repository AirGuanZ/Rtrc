#include <Rtrc/ToolKit/Camera/Camera.h>

RTRC_BEGIN

void Camera::SetLookAt(const Vector3f &position, const Vector3f &up, const Vector3f &target)
{
    position_ = position;

    const Vector3f ez = Normalize(target - position);
    const Vector3f ex = Normalize(Cross(up, ez));
    const Vector3f ey = Normalize(Cross(ez, ex));

    // Rotation matrix:
    // 
    //   [ ex.x ey.x ez.x ]
    //   [ ex.y ey.y ez.y ]
    //   [ ex.z ey.z ez.z ]
    //
    //   [ CyCz + SxSySz  -SzCy + SxSyCz SyCx ]
    //   [ SzCx           CxCz           -Sx  ]
    //   [ -SyCz + SxSzCy SySz + SxCyCz  CxCy ]

    const float sx = -ez.y;
    if(std::abs(std::abs(sx) - 1) > 1e-3f)
    {
        const float x1 = std::asin(std::clamp(sx, -1.0f, 1.0f));
        const float cx1 = std::cos(x1);
        const float z1 = Atan2Safe(ex.y / cx1, ey.y / cx1);
        const float y1 = Atan2Safe(ez.x / cx1, ez.z / cx1);
        // const float x2 = PI - x1;
        // const float cx2 = std::cos(x2);
        // const float z2 = std::atan2(ex.y / cx2, ey.y / cx2);
        // const float y2 = std::atan2(ez.x / cx2, ez.z / cx2);
        // Both (x1, y1, z1) and (x2, y2, z2) are valid. We choose (x1, y1, z1) to ensure x1 is between [-PI/2, PI/2]
        rotation_ = { x1, y1, z1 };
    }
    else // cx = 0, sy = 0, cy = 1
    {
        if(sx > 0) // sx = 1
        {
            const float sz = ex.z;
            const float cz = ey.z;
            const float z = Atan2Safe(sz, cz);
            rotation_ = { PI / 2, 0, z };
        }
        else // sx = -1
        {
            const float sz = -ex.z;
            const float cz = -ey.z;
            const float z = Atan2Safe(sz, cz);
            rotation_ = { -PI / 2, 0, z };
        }
    }
}

void Camera::UpdateDerivedData()
{
    const Matrix4x4f rotateToWorld =
        Matrix4x4f::RotateY(rotation_.y) *
        Matrix4x4f::RotateX(rotation_.x) *
        Matrix4x4f::RotateZ(rotation_.z);

    front_ = (rotateToWorld * Vector4f(0, 0, 1, 0)).xyz();
    up_    = (rotateToWorld * Vector4f(0, 1, 0, 0)).xyz();
    left_  = (rotateToWorld * Vector4f(1, 0, 0, 0)).xyz();
    
    cameraToWorld_ = Matrix4x4f::Translate(position_) * rotateToWorld;
    worldToCamera_ = Transpose(rotateToWorld) * Matrix4x4f::Translate(-position_);

    cameraToClip_ = Matrix4x4f::Perspective(fovYRad_, aspectRatio_, near_, far_);
    clipToCamera_ = Inverse(cameraToClip_); // TODO: optimize

    worldToClip_ = cameraToClip_ * worldToCamera_;
    clipToWorld_ = cameraToWorld_ * clipToCamera_;

    // TODO: optimize
    const Vector4f cameraSpaceRays[4] =
    {
        clipToCamera_ * Vector4f(-1.0f, +1.0f, 1.0f, 1.0f),
        clipToCamera_ * Vector4f(+1.0f, +1.0f, 1.0f, 1.0f),
        clipToCamera_ * Vector4f(-1.0f, -1.0f, 1.0f, 1.0f),
        clipToCamera_ * Vector4f(+1.0f, -1.0f, 1.0f, 1.0f)
    };
    cameraRays_[0] = Normalize(cameraSpaceRays[0].xyz() / cameraSpaceRays[0].w);
    cameraRays_[1] = Normalize(cameraSpaceRays[1].xyz() / cameraSpaceRays[0].w);
    cameraRays_[2] = Normalize(cameraSpaceRays[2].xyz() / cameraSpaceRays[0].w);
    cameraRays_[3] = Normalize(cameraSpaceRays[3].xyz() / cameraSpaceRays[0].w);

    const Vector4f worldSpaceRays[4] =
    {
        clipToWorld_ *Vector4f(-1.0f, +1.0f, 1.0f, 1.0f),
        clipToWorld_ *Vector4f(+1.0f, +1.0f, 1.0f, 1.0f),
        clipToWorld_ *Vector4f(-1.0f, -1.0f, 1.0f, 1.0f),
        clipToWorld_ * Vector4f(+1.0f, -1.0f, 1.0f, 1.0f)
    };
    worldRays_[0] = worldSpaceRays[0].xyz() / worldSpaceRays[0].w - position_;
    worldRays_[1] = worldSpaceRays[1].xyz() / worldSpaceRays[0].w - position_;
    worldRays_[2] = worldSpaceRays[2].xyz() / worldSpaceRays[0].w - position_;
    worldRays_[3] = worldSpaceRays[3].xyz() / worldSpaceRays[0].w - position_;
    worldRays_[0] = 1.0f / Dot(worldRays_[0], front_) * worldRays_[0];
    worldRays_[1] = 1.0f / Dot(worldRays_[1], front_) * worldRays_[1];
    worldRays_[2] = 1.0f / Dot(worldRays_[2], front_) * worldRays_[2];
    worldRays_[3] = 1.0f / Dot(worldRays_[3], front_) * worldRays_[3];
}

RTRC_END
