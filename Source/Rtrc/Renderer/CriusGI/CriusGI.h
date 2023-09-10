#pragma once

#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

class RenderCamera;

class CriusGI : public RenderAlgorithm
{
public:

    struct Parameters
    {
        
    };

    struct PerCameraData
    {
        RC<StatefulTexture> persistentRngState;
        RG::TextureResource *rngState = nullptr;

        RG::TextureResource *rawTracedA = nullptr;
        RG::TextureResource *rawTracedB = nullptr;

        RC<StatefulTexture> persistentReservoirs;
        RG::TextureResource *reservoirs = nullptr;

        // In half resolution
        RG::TextureResource *indirectDiffuse = nullptr;
    };

    using RenderAlgorithm::RenderAlgorithm;

    const Parameters &GetParameters() const { return parameters_; }
          Parameters &GetParameters()       { return parameters_; }

    void Render(RG::RenderGraph &renderGraph, RenderCamera &camera, const GBuffers &gbuffers) const;

    void ClearFrameData(PerCameraData &data) const;

private:

    void InitializeData(RG::RenderGraph &renderGraph, const Vector2u &framebufferSize, PerCameraData &data) const;

    enum PassIndex
    {
        PassIndex_Trace,
        PassIndex_Count
    };

    Parameters parameters_;
};

RTRC_RENDERER_END
