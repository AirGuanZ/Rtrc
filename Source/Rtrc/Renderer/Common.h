#pragma once

#include <Graphics/Device/Device.h>
#include <Graphics/RenderGraph/Graph.h>
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
    RG::TextureResource *albedoMetallic = nullptr;
    RG::TextureResource *roughness      = nullptr;
    RG::TextureResource *depthStencil   = nullptr;

    RG::TextureResource *currDepth = nullptr;
    RG::TextureResource *prevDepth = nullptr;

    RG::TextureResource *currNormal = nullptr;
    RG::TextureResource *prevNormal = nullptr;
};

class RenderAlgorithm : public Uncopyable
{
public:

    explicit RenderAlgorithm(Ref<ResourceManager> resources)
        : device_(resources->GetDevice()), resources_(resources)
    {
        
    }

protected:

    template<TemplateStringParameter S>
    RC<Shader> GetStaticShader() const
    {
        return resources_->GetShaderManager()->GetShader<S>();
    }

    template<TemplateStringParameter S>
    RC<Shader> GetStaticShader(const FastKeywordContext &context) const
    {
        return resources_->GetShaderManager()->GetShaderTemplate<S>()->GetVariant(context);
    }

    Ref<Device>          device_;
    Ref<ResourceManager> resources_;
};

RTRC_RENDERER_END
