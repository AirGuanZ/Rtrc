#pragma once

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Resource/BuiltinResources.h>
#include <Rtrc/Renderer/Scene/CachedScene.h>

RTRC_RENDERER_BEGIN

class PathTracer
{
public:

    struct Parameters
    {
        uint32_t maxDepth = 5;
    };

    struct RGOutput
    {
        RG::TextureResource *indirectDiffuse = nullptr;
    };

    PathTracer(ObserverPtr<Device> device, ObserverPtr<BuiltinResourceManager> builtinResources);

    const Parameters &GetParameters() const;
          Parameters &GetParameters();

    RGOutput Render(RG::RenderGraph &renderGraph, const CachedCamera &camera) const;

private:

    ObserverPtr<Device>                 device_;
    ObserverPtr<BuiltinResourceManager> builtinResources_;

    Parameters parameters_;
};

RTRC_RENDERER_END
