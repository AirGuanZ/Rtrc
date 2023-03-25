#pragma once

#include <Rtrc/Renderer/RenderLoopScene/RenderLoopMeshManager.h>

RTRC_RENDERER_BEGIN

class RenderLoopScene : public Uncopyable
{
public:

    struct StaticMeshRecord
    {
        const StaticMeshRenderProxy             *proxy = nullptr;
        const RenderLoopMeshManager::CachedMesh *cachedMesh = nullptr;

    };
};

RTRC_RENDERER_END
