#pragma once

#include <Rtrc/Math/Matrix4x4.h>

RTRC_BEGIN

class SceneObject
{
public:

    void SetWorldMatrix(const Matrix4x4f &localToWorld, const Matrix4x4f &worldToLocal)
    {
        localToWorld_ = localToWorld;
        worldToLocal_ = worldToLocal;
    }

    const Matrix4x4f &GetLocalToWorld() const { return localToWorld_; }
    const Matrix4x4f &GetWorldToLocal() const { return worldToLocal_; }

private:

    Matrix4x4f localToWorld_;
    Matrix4x4f worldToLocal_;
};

RTRC_END
