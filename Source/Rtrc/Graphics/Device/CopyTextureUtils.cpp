#include <Rtrc/Graphics/Device/CopyTextureUtils.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Shader/ShaderCompiler.h>

RTRC_BEGIN

namespace CopyTextureUtilsDetail
{
    const char *SHADER_SOURCE_POINT_SAMPLING = R"___(
#vertex VSMain
#pixel PSMain
rtrc_group(Pass)
{
    rtrc_define(Texture2D<float4>, Src)
};
#if USE_POINT_SAMPLING
rtrc_sampler(Sampler, filter = point, address_u = repeat, address_v = clamp)
#else
rtrc_sampler(Sampler, filter = linear, address_u = repeat, address_v = clamp)
#endif
struct Vs2Ps
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD;
};
Vs2Ps VSMain(uint id : SV_VertexID)
{
    float2 uv = float2((id << 1) & 2, id & 2);
    Vs2Ps output;
    output.position = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    output.uv = uv;
    return output;
}
float4 PSMain(Vs2Ps input) : SV_Target
{
    return Src.SampleLevel(Sampler, input.uv, 0);
}
)___";

    rtrc_group(Pass)
    {
        rtrc_define(Texture2D, Src);
    };

} // namespace namespace CopyTextureUtilsDetail

CopyTextureUtils::CopyTextureUtils(ObserverPtr<Device> device)
    : device_(device)
{
    ShaderCompiler shaderCompiler;
    shaderCompiler.SetDevice(device);
    const ShaderCompiler::ShaderSource source = { .source = CopyTextureUtilsDetail::SHADER_SOURCE_POINT_SAMPLING };
    shaderUsingPointSampling_ = shaderCompiler.Compile(source, { { "USE_POINT_SAMPLING", "1" } }, RTRC_DEBUG);
    shaderUsingLinearSampling_ = shaderCompiler.Compile(source, { { "USE_POINT_SAMPLING", "0" } }, RTRC_DEBUG);
    bindingGroupLayout_ = device->CreateBindingGroupLayout<CopyTextureUtilsDetail::Pass>();
}

void CopyTextureUtils::RenderQuad(
    CommandBuffer    &commandBuffer,
    const TextureSrv &src,
    const TextureRtv &dst,
    SamplingMethod    samplingMethod)
{
    commandBuffer.BeginRenderPass(ColorAttachment
    {
        .renderTargetView = dst,
        .loadOp           = AttachmentLoadOp::DontCare,
        .storeOp          = AttachmentStoreOp::Store
    });
    RTRC_SCOPE_EXIT{ commandBuffer.EndRenderPass(); };

    auto dstTexture = dst.GetTexture().get();
    auto pipeline = GetPipeline(dstTexture->GetFormat(), samplingMethod == Point);
    commandBuffer.BindGraphicsPipeline(pipeline);

    CopyTextureUtilsDetail::Pass passData;
    passData.Src = src;
    auto passGroup = device_->CreateBindingGroup(passData, bindingGroupLayout_);
    commandBuffer.BindGraphicsGroup(0, passGroup);

    commandBuffer.SetViewports(dstTexture->GetViewport());
    commandBuffer.SetScissors(dstTexture->GetScissor());
    commandBuffer.Draw(3, 1, 0, 0);
}

const RC<GraphicsPipeline> &CopyTextureUtils::GetPipeline(RHI::Format format, bool pointSampling)
{
    auto &map = pointSampling ? dstFormatToPipelineUsingPointSampling_ : dstFormatToPipelineUsingLinearSampling_;

    {
        std::shared_lock lock(mutex_);
        if(auto it = map.find(format); it != map.end())
        {
            return it->second;
        }
    }

    std::lock_guard lock(mutex_);
    if(auto it = map.find(format); it != map.end())
    {
        return it->second;
    }

    auto &shader = pointSampling ? shaderUsingPointSampling_ : shaderUsingLinearSampling_;
    auto newPipeline = device_->CreateGraphicsPipeline(GraphicsPipeline::Desc
    {
        .shader                 = shader,
        .colorAttachmentFormats = { format }
    });
    auto it = map.insert({ format, std::move(newPipeline) }).first;
    return it->second;
}

RTRC_END
