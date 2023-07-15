#pragma once

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Resource/BuiltinResources.h>
#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

class RenderCamera;

class PathTracer : public RenderAlgorithm
{
public:

    struct Parameters
    {
        uint32_t maxDepth = 5;
    };

    struct PerCameraData
    {
        RC<StatefulTexture> persistentRngState;
        RG::TextureResource *indirectDiffuse = nullptr;
    };

    using RenderAlgorithm::RenderAlgorithm;

    const Parameters &GetParameters() const { return parameters_; }
          Parameters &GetParameters()       { return parameters_; }
          
    void Render(
        RG::RenderGraph &renderGraph,
        RenderCamera    &camera,
        const GBuffers  &gbuffers,
        const Vector2u  &framebufferSize) const;

    void ClearFrameData(PerCameraData &data) const;

private:

    enum PassIndex
    {
        PassIndex_Trace,
        PassIndex_Count
    };
    
    ObserverPtr<Device>                 device_;
    ObserverPtr<BuiltinResourceManager> builtinResources_;

    Parameters parameters_;
};

RTRC_RENDERER_END
