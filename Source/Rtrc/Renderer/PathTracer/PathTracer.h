#pragma once

#include <Graphics/Device/Device.h>
#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

class RenderCamera;

// Test only. Will be replaced by a more general solution.
class PathTracer : public RenderAlgorithm
{
public:

    struct Parameters
    {
        uint32_t maxDepth = 5;
    };

    struct PerCameraData
    {
        RC<StatefulTexture> rngState;

        RC<StatefulTexture> prev; // Temporal-filtered result
        RC<StatefulTexture> curr;

        RG::TextureResource *indirectDiffuse = nullptr;
    };

    using RenderAlgorithm::RenderAlgorithm;

    const Parameters &GetParameters() const { return parameters_; }
          Parameters &GetParameters()       { return parameters_; }
          
    void Render(
        RG::RenderGraph &renderGraph,
        RenderCamera    &camera,
        bool             clearBeforeRender) const;

    void ClearFrameData(PerCameraData &data) const;

private:

    enum PassIndex
    {
        PassIndex_Trace,
        PassIndex_TemporalFilter,
        PassIndex_SpatialFilter,
        PassIndex_Count
    };
    
    Parameters parameters_;
};

RTRC_RENDERER_END
