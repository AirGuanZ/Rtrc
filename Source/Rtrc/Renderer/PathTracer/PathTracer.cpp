#include <Rtrc/Renderer/GBufferBinding.h>
#include <Rtrc/Renderer/PathTracer/PathTracer.h>

RTRC_RENDERER_BEGIN

PathTracer::PathTracer(ObserverPtr<Device> device, ObserverPtr<BuiltinResourceManager> builtinResources)
    : device_(device), builtinResources_(builtinResources)
{
    
}

const PathTracer::Parameters &PathTracer::GetParameters() const
{
    return parameters_;
}

PathTracer::Parameters &PathTracer::GetParameters()
{
    return parameters_;
}

PathTracer::RGOutput PathTracer::Render(
    RG::RenderGraph    &renderGraph,
    const RenderCamera &camera,
    const GBuffers     &gbuffers,
    const Vector2u     &framebufferSize) const
{
    auto indirectDiffuse = renderGraph.CreateTexture(RHI::TextureDesc
    {
        .dim    = RHI::TextureDimension::Tex2D,
        .format = RHI::Format::R8G8B8A8_UNorm,
        .width  = framebufferSize.x,
        .height = framebufferSize.y,
        .usage  = RHI::TextureUsage::UnorderAccess | RHI::TextureUsage::ShaderResource
    }, "PathTracer-IndirectDiffuse");

    // TODO
    return {};
}

RTRC_RENDERER_END
