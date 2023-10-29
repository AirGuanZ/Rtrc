#pragma once

#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

class SVGF : public RenderAlgorithm
{
public:

    struct PerCameraData
    {
        RC<StatefulTexture> prevColor;
        RC<StatefulTexture> prevMoments;

        RC<StatefulTexture> currColor;
        RC<StatefulTexture> currMoments;

        RG::TextureResource *finalColor = nullptr;
    };

    struct Settings
    {
        float temporalFilterAlpha = 0.1f;
        unsigned int spatialFilterIterations = 2;
    };

    using RenderAlgorithm::RenderAlgorithm;

    RTRC_SET_GET(Settings, Settings, settings_)

    void Render(
        ObserverPtr<RG::RenderGraph> renderGraph,
        ObserverPtr<RenderCamera>    renderCamera,
        PerCameraData               &data,
        RG::TextureResource         *inputColor) const;

    void ClearFrameData(PerCameraData &data) const;

private:

    Settings settings_;
};

RTRC_RENDERER_END
