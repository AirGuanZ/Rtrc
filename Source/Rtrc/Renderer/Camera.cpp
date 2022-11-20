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

    cameraToWorldMatrix_ = Matrix4x4f::Translate(position_) * rotateToWorld;
    worldToCameraMatrix_ = Transpose(rotateToWorld) * Matrix4x4f::Translate(-position_);

    cameraToClipMatrix_ = Matrix4x4f::Perspective(
        projParams_.fovYRad, projParams_.wOverH, projParams_.nearPlane, projParams_.farPlane);
    clipToCameraMatrix_ = Inverse(cameraToClipMatrix_); // TODO: optimize

    worldToClipMatrix_ = cameraToClipMatrix_ * worldToCameraMatrix_;
    clipToWorldMatrix_ = cameraToWorldMatrix_ * clipToCameraMatrix_;

    ClearDrity();
}

void Camera::SetUpRenderCamera(RenderCamera &renderCamera) const
{
    AssertNotDirty();
    CameraConstantBuffer &data = renderCamera.data_;
    data.worldToCameraMatrix = worldToCameraMatrix_;
    data.cameraToWorldMatrix = cameraToWorldMatrix_;
    data.cameraToClipMatrix = cameraToClipMatrix_;
    data.clipToCameraMatrix = clipToCameraMatrix_;
    data.worldToClipMatrix = worldToClipMatrix_;
    data.clipToWorldMatrix = clipToWorldMatrix_;

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
    data.worldRays[0] = Normalize(cameraSpaceRays[0].xyz() / cameraSpaceRays[0].w - position_);
    data.worldRays[1] = Normalize(cameraSpaceRays[1].xyz() / cameraSpaceRays[0].w - position_);
    data.worldRays[2] = Normalize(cameraSpaceRays[2].xyz() / cameraSpaceRays[0].w - position_);
    data.worldRays[3] = Normalize(cameraSpaceRays[3].xyz() / cameraSpaceRays[0].w - position_);
}

RTRC_END
