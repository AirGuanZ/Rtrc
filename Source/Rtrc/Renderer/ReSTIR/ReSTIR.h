#pragma once

#include <Rtrc/Renderer/SVGF/SVGF.h>

RTRC_RENDERER_BEGIN

class ReSTIR : public RenderAlgorithm
{
public:

    struct Settings
    {
        bool enableTemporalReuse = true;
        unsigned int M  = 4;
        unsigned int maxM = 64;

        unsigned int N = 5; // N = 0 implies no spatial reuse
        float radius = 15;

        float svgfTemporalFilterAlpha = 0.1f;
        unsigned int svgfSpatialFilterIterations = 2;
    };

    struct PerCameraData
    {
        SVGF::PerCameraData svgf;

        RC<StatefulTexture> pcgState;
        RC<StatefulTexture> prevReservoirs;
        RC<StatefulTexture> currReservoirs;

        RG::TextureResource *directIllum = nullptr;
    };

    explicit ReSTIR(ObserverPtr<ResourceManager> resources);

    RTRC_SET_GET(Settings, Settings, settings_)

    void Render(RenderCamera &renderCamera, RG::RenderGraph &renderGraph);

    void ClearFrameData(PerCameraData &data) const;

private:

    Settings   settings_;
    RC<Buffer> dummyLightBuffer_;
    SVGF       svgf_;
};

RTRC_RENDERER_END
