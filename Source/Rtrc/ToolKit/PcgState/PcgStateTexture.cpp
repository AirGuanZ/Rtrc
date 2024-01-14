#include <Rtrc/Graphics/RenderGraph/Graph.h>
#include <Rtrc/ToolKit/PcgState/PcgStateTexture.h>

#include "PcgStateTexture.shader.outh"

RTRC_RENDERER_BEGIN

RG::TextureResource *Prepare2DPcgStateTexture(
    RG::RenderGraph     &renderGraph,
    Ref<ResourceManager> resources,
    RC<StatefulTexture> &tex,
    const Vector2u      &size)
{
    if(tex && tex->GetSize() == size)
    {
        return renderGraph.RegisterTexture(tex);
    }

    auto device = renderGraph.GetDevice();
    tex = device->CreateStatefulTexture(RHI::TextureDesc
    {
        .dim    = RHI::TextureDimension::Tex2D,
        .format = RHI::Format::R32_UInt,
        .width  = size.x,
        .height = size.y,
        .usage  = RHI::TextureUsage::UnorderAccess | RHI::TextureUsage::ShaderResource
    });
    tex->SetName("PcgState");
    auto ret = renderGraph.RegisterTexture(tex);

    using Shader = RtrcShader::Builtin::InitializePcgState2D;
    auto initPass = renderGraph.CreatePass(Shader::Name);
    initPass->Use(ret, RG::CS_RWTexture_WriteOnly);
    initPass->SetCallback([ret, size, device, resources]
    {
        Shader::Pass passData;
        passData.Output = ret;
        passData.resolution = size;
        auto passGroup = device->CreateBindingGroupWithCachedLayout(passData);

        auto shader = resources->GetStaticShader<Shader::Name>();

        auto &commandBuffer = RG::GetCurrentCommandBuffer();
        commandBuffer.BindComputePipeline(shader->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(size);
    });

    return ret;
}

RTRC_RENDERER_END
