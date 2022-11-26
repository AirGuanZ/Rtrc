#include <Rtrc/Renderer/Camera.h>

RTRC_BEGIN

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

    ClearDrity();
}

RTRC_END
