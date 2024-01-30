#pragma once

#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_RENDERER_BEGIN

RGPass PrepareThreadGroupCount1D(
    GraphRef        renderGraph,
    const RGBufSrv &threadCountBuffer,
    const RGBufUav &threadGroupCountBuffer,
    int             threadGroupSize);

RTRC_RENDERER_END
