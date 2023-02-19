#include <Rtrc/Renderer/Camera/Camera.h>

RTRC_BEGIN

void Camera::SetLookAt(const Vector3f &up, const Vector3f &destination)
{
    auto SafeAtan2 = [](float y, float x)
    {
        return (x == 0 && y == 0) ? 0.0f : std::atan2(y, x);
    };

    const Vector3f ez = Normalize(destination - position_);
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
        const float z1 = SafeAtan2(ex.y / cx1, ey.y / cx1);
        const float y1 = SafeAtan2(ez.x / cx1, ez.z / cx1);
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
            const float z = SafeAtan2(sz, cz);
            rotation_ = { PI / 2, 0, z };
        }
        else // sx = -1
        {
            const float sz = -ex.z;
            const float cz = -ey.z;
            const float z = SafeAtan2(sz, cz);
            rotation_ = { -PI / 2, 0, z };
        }
    }
}

void Camera::CalculateDerivedData()
{
    const Matrix4x4f rotateToWorld =
        Matrix4x4f::RotateY(rotation_.y) *
        Matrix4x4f::RotateX(rotation_.x) *
        Matrix4x4f::RotateZ(rotation_.z);

    forward_ = (rotateToWorld * Vector4f(0, 0, 1, 0)).xyz();
    up_      = (rotateToWorld * Vector4f(0, 1, 0, 0)).xyz();
    left_    = (rotateToWorld * Vector4f(1, 0, 0, 0)).xyz();

    auto &data = constantBufferData_;
    data.cameraToWorldMatrix = Matrix4x4f::Translate(position_) * rotateToWorld;
    data.worldToCameraMatrix = Transpose(rotateToWorld) * Matrix4x4f::Translate(-position_);

    data.cameraToClipMatrix = Matrix4x4f::Perspective(
        projParams_.fovYRad, projParams_.wOverH, projParams_.nearPlane, projParams_.farPlane);
    data.clipToCameraMatrix = Inverse(data.cameraToClipMatrix); // TODO: optimize

    data.worldToClipMatrix = data.cameraToClipMatrix * data.worldToCameraMatrix;
    data.clipToWorldMatrix = data.cameraToWorldMatrix * data.clipToCameraMatrix;

    // TODO: optimize
    const Vector4f cameraSpaceRays[4] =
    {
        data.clipToCameraMatrix * Vector4f(-1.0f, +1.0f, 1.0f, 1.0f),
        data.clipToCameraMatrix * Vector4f(+1.0f, +1.0f, 1.0f, 1.0f),
        data.clipToCameraMatrix * Vector4f(-1.0f, -1.0f, 1.0f, 1.0f),
        data.clipToCameraMatrix * Vector4f(+1.0f, -1.0f, 1.0f, 1.0f)
    };
    data.cameraRays[0] = Normalize(cameraSpaceRays[0].xyz() / cameraSpaceRays[0].w);
    data.cameraRays[1] = Normalize(cameraSpaceRays[1].xyz() / cameraSpaceRays[0].w);
    data.cameraRays[2] = Normalize(cameraSpaceRays[2].xyz() / cameraSpaceRays[0].w);
    data.cameraRays[3] = Normalize(cameraSpaceRays[3].xyz() / cameraSpaceRays[0].w);

    const Vector4f worldSpaceRays[4] =
    {
        data.clipToWorldMatrix * Vector4f(-1.0f, +1.0f, 1.0f, 1.0f),
        data.clipToWorldMatrix * Vector4f(+1.0f, +1.0f, 1.0f, 1.0f),
        data.clipToWorldMatrix * Vector4f(-1.0f, -1.0f, 1.0f, 1.0f),
        data.clipToWorldMatrix * Vector4f(+1.0f, -1.0f, 1.0f, 1.0f)
    };
    data.worldRays[0] = Normalize(worldSpaceRays[0].xyz() / worldSpaceRays[0].w - position_);
    data.worldRays[1] = Normalize(worldSpaceRays[1].xyz() / worldSpaceRays[0].w - position_);
    data.worldRays[2] = Normalize(worldSpaceRays[2].xyz() / worldSpaceRays[0].w - position_);
    data.worldRays[3] = Normalize(worldSpaceRays[3].xyz() / worldSpaceRays[0].w - position_);
}

RTRC_END
