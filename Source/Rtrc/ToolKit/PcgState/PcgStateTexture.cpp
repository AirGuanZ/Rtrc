#include <Rtrc/Graphics/RenderGraph/Graph.h>
#include <Rtrc/ToolKit/PcgState/PcgStateTexture.h>

#include "PcgStateTexture.shader.outh"

RTRC_RENDERER_BEGIN

RGTexture Prepare2DPcgStateTexture(
    GraphRef             renderGraph,
    RC<StatefulTexture> &tex,
    const Vector2u      &size)
{
    if(tex && tex->GetSize() == size)
    {
        return renderGraph->RegisterTexture(tex);
    }

    auto device = renderGraph->GetDevice();
    tex = device->CreateStatefulTexture(RHI::TextureDesc
    {
        .dim    = RHI::TextureDimension::Tex2D,
        .format = RHI::Format::R32_UInt,
        .width  = size.x,
        .height = size.y,
        .usage  = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource
    });
    tex->SetName("PcgState");
    auto ret = renderGraph->RegisterTexture(tex);

    using Shader = RtrcShader::Builtin::InitializePcgState2D;
    auto initPass = renderGraph->CreatePass(Shader::Name);
    initPass->Use(ret, RG::CS_RWTexture_WriteOnly);
    initPass->SetCallback([ret, size, device]
    {
        Shader::Pass passData;
        passData.Output = ret;
        passData.resolution = size;
        auto passGroup = device->CreateBindingGroupWithCachedLayout(passData);

        auto shader = device->GetShader<Shader::Name>();

        auto &commandBuffer = RGGetCommandBuffer();
        commandBuffer.BindComputePipeline(shader->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(size);
    });

    return ret;
}

RTRC_RENDERER_END
