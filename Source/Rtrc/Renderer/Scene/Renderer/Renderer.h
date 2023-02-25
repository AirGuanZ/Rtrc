#pragma once

#include <Rtrc/Math/AABB.h>
#include <Rtrc/Math/Matrix4x4.h>
#include <Rtrc/Renderer/Scene/Hierarchy/SceneObject.h>
#include <Rtrc/Utility/Memory/Arena.h>

RTRC_BEGIN

class RendererProxy
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

class Renderer : public SceneObject
{
public:

    virtual RendererProxy *CreateProxy(LinearAllocator &proxyAllocator) const = 0;
};

RTRC_END
