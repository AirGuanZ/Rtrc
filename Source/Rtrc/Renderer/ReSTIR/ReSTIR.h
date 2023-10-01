#pragma once

#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

class ReSTIR : public RenderAlgorithm
{
public:

    struct PerCameraData
    {
        // x: light index
        // y: light uv
        // z: weight sum
        // w: sample count
        RC<StatefulTexture> reservoirs;
        RG::TextureResource *directIllum = nullptr;
    };

    using RenderAlgorithm::RenderAlgorithm;

    void Render(RenderCamera &renderCamera, RG::RenderGraph &renderGraph, const GBuffers &gbuffers);

    void ClearFrameData(PerCameraData &data) const;

private:

    RC<Buffer> dummyLightBuffer_;
};

RTRC_RENDERER_END
