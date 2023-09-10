#pragma once

#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

class GBufferVisualizer : public RenderAlgorithm
{
public:

    enum class Mode
    {
        Normal
    };

    using RenderAlgorithm::RenderAlgorithm;

    void Render(
        Mode                 mode,
        RG::RenderGraph     &renderGraph,
        const GBuffers      &gbuffers,
        RG::TextureResource *renderTarget);
};

RTRC_RENDERER_END
