#pragma once

#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

class Crius : public RenderAlgorithm
{
public:

    struct Settings
    {
        
    };

    struct PerCameraData
    {
        RC<StatefulBuffer> surfelBuffer;
    };

    using RenderAlgorithm::RenderAlgorithm;

    RTRC_SET_GET(Settings, Settings, settings_)

    void Render(
        ObserverPtr<RG::RenderGraph> renderGraph,
        ObserverPtr<RenderCamera>    renderCamera) const;

private:

    Settings settings_;
};

RTRC_RENDERER_END
