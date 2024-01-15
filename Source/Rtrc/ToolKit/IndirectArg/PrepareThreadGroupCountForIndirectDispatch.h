#pragma once

#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_RENDERER_BEGIN

RG::Pass *PrepareThreadGroupCount1D(
    Ref<RG::RenderGraph>         renderGraph,
    const RG::BufferResourceSrv &threadCountBuffer,
    const RG::BufferResourceUav &threadGroupCountBuffer,
    int                          threadGroupSize);

RTRC_RENDERER_END
