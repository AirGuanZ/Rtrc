#pragma once

#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

class ReSTIR : public RenderAlgorithm
{
public:

    struct PerCameraData
    {
        RC<StatefulTexture> pcgState;
        RC<StatefulTexture> prevReservoirs;
        RC<StatefulTexture> currReservoirs;

        RG::TextureResource *directIllum = nullptr;
    };

    using RenderAlgorithm::RenderAlgorithm;

    void Render(RenderCamera &renderCamera, RG::RenderGraph &renderGraph, const GBuffers &gbuffers);

    void ClearFrameData(PerCameraData &data) const;

private:

    RC<Buffer> dummyLightBuffer_;
    unsigned int M_ = 2;
    unsigned int maxM_ = 32;
    bool enableTemporalReuse_ = true;
};

RTRC_RENDERER_END
