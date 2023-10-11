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

    void SetM(unsigned int M);
    void SetMaxM(unsigned int maxM);
    void SetN(unsigned int N);
    void SetRadius(float radius);
    void SetEnableTemporalReuse(bool value);

    void Render(RenderCamera &renderCamera, RG::RenderGraph &renderGraph, const GBuffers &gbuffers);

    void ClearFrameData(PerCameraData &data) const;

private:

    RC<Buffer> dummyLightBuffer_;
    unsigned int M_ = 2;
    unsigned int maxM_ = 32;
    unsigned int N_ = 5;
    float radius_ = 5;
    bool enableTemporalReuse_ = true;
};

RTRC_RENDERER_END
