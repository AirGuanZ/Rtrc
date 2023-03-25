#pragma once

#include <Rtrc/Math/AABB.h>
#include <Rtrc/Math/Matrix4x4.h>
#include <Rtrc/Scene/Hierarchy/SceneObject.h>
#include <Rtrc/Utility/Memory/Arena.h>

RTRC_BEGIN

class RenderProxy
{
public:

    enum class Type
    {
        None,
        StaticMesh
    };

    Type       type = Type::None;
    Matrix4x4f localToWorld;
    Matrix4x4f worldToLocal;
    AABB3f     localBound;
    AABB3f     worldBound;
};

class RenderObject : public SceneObject
{
public:

    virtual RenderProxy *CreateProxy(LinearAllocator &proxyAllocator) const = 0;
};

RTRC_END
