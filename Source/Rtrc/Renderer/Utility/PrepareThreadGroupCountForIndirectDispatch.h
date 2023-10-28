#pragma once

#include <Graphics/RenderGraph/Graph.h>

RTRC_RENDERER_BEGIN

RG::Pass *PrepareThreadGroupCount1D(
    ObserverPtr<ResourceManager> resources,
    ObserverPtr<RG::RenderGraph> renderGraph,
    const RG::BufferResourceSrv &threadCountBuffer,
    const RG::BufferResourceUav &threadGroupCountBuffer,
    int                          threadGroupSize);

RTRC_RENDERER_END
