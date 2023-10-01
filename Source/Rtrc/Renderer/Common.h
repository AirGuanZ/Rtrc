#pragma once

#include <Graphics/Device/Device.h>
#include <Rtrc/Resource/ResourceManager.h>
#include <Rtrc/Scene/Camera/Camera.h>
#include <Rtrc/Scene/Scene.h>

RTRC_RENDERER_BEGIN

class RenderScene;
class RenderCamera;

enum class StencilBit : uint8_t
{
    None    = 0,
    Regular = 1 << 0,
};

struct GBuffers
{
    RG::TextureResource *normal         = nullptr;
    RG::TextureResource *albedoMetallic = nullptr;
    RG::TextureResource *roughness      = nullptr;
    RG::TextureResource *depthStencil   = nullptr;

    RG::TextureResource *currDepth = nullptr;
    RG::TextureResource *prevDepth = nullptr;
};

class RenderAlgorithm : public Uncopyable
{
public:

    explicit RenderAlgorithm(ObserverPtr<ResourceManager> resources)
        : device_(resources->GetDevice()), resources_(resources)
    {
        
    }

protected:

    ObserverPtr<Device>          device_;
    ObserverPtr<ResourceManager> resources_;
};

RTRC_RENDERER_END
