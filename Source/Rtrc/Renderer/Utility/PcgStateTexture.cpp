#include <Rtrc/Graphics/Resource/MaterialManager.h>
#include <Rtrc/Renderer/Utility/PcgStateTexture.h>

#include "Rtrc/Graphics/Resource/ResourceManager.h"

RTRC_RENDERER_BEGIN
    RG::TextureResource *Prepare2DPcgStateTexture(
    RG::RenderGraph             &renderGraph,
    ObserverPtr<ResourceManager> materials,
    RC<StatefulTexture>         &tex,
    const Vector2u              &size)
{
    rtrc_group(Pass)
    {
        rtrc_define(RWTexture2D, Output);
        rtrc_uniform(uint2, resolution);
    };

    if(tex && tex->GetSize() == size)
    {
        return renderGraph.RegisterTexture(tex);
    }

    auto device = renderGraph.GetDevice();
    
    tex = device->CreatePooledTexture(RHI::TextureDesc
    {
        .dim    = RHI::TextureDimension::Tex2D,
        .format = RHI::Format::R32_UInt,
        .width  = size.x,
        .height = size.y,
        .usage  = RHI::TextureUsage::UnorderAccess | RHI::TextureUsage::ShaderResource
    });
    tex->SetName("PcgState");
    auto ret = renderGraph.RegisterTexture(tex);

    auto initPass = renderGraph.CreatePass("InitializePcgState");
    initPass->Use(ret, RG::CS_RWTexture_WriteOnly);
    initPass->SetCallback([ret, size, device, materials]
    {
        Pass passData;
        passData.Output = ret;
        passData.resolution = size;
        auto passGroup = device->CreateBindingGroupWithCachedLayout(passData);

        auto shader = materials->GetMaterialManager()->GetCachedShader<"Builtin/Utility/InitializePcgState2D">();
        auto &pipeline = shader->GetComputePipeline();

        auto &commandBuffer = RG::GetCurrentCommandBuffer();
        commandBuffer.BindComputePipeline(pipeline);
        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(size);
    });

    return ret;
}

RTRC_RENDERER_END
