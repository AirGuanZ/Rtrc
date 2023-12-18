#pragma once

#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

class DeferredLighting : public RenderAlgorithm
{
public:

    using RenderAlgorithm::RenderAlgorithm;

    void Render(
        Ref<RenderCamera>    renderCamera,
        Ref<RG::RenderGraph> renderGraph,
        RG::TextureResource         *directIllum,
        RG::TextureResource         *sky,
        RG::TextureResource         *renderTarget) const;
};

RTRC_RENDERER_END
