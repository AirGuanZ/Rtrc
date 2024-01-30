#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Device/Utility/CopyTextureUtils.h>
#include <Rtrc/Graphics/Shader/StandaloneCompiler.h>

RTRC_BEGIN

namespace CopyTextureUtilsDetail
{
    const char *SHADER_SOURCE_POINT_SAMPLING = R"___(
rtrc_vertex(VSMain)
rtrc_pixel(PSMain)
rtrc_group(Pass)
{
    rtrc_define(Texture2D<float4>, Src)
#if USE_GAMMA
    rtrc_uniform(float, gamma)
#endif
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
    float4 result = Src.SampleLevel(Sampler, input.uv, 0);
#if USE_GAMMA
    result.xyz = pow(result.xyz, Pass.gamma);
#endif
    return result;
}
)___";

    template<bool EnableGamma>
    rtrc_group(Pass)
    {
        rtrc_define(Texture2D, Src);
        rtrc_cond_uniform(EnableGamma, float, gamma);
    };

} // namespace namespace CopyTextureUtilsDetail

CopyTextureUtils::CopyTextureUtils(Ref<Device> device)
    : device_(device)
{
    StandaloneShaderCompiler shaderCompiler;
    shaderCompiler.SetDevice(device);
    const std::string source = CopyTextureUtilsDetail::SHADER_SOURCE_POINT_SAMPLING;
    shaderPointSampling_       = shaderCompiler.Compile(source, { { "USE_POINT_SAMPLING", "1" }, { "USE_GAMMA", "0" } }, RTRC_DEBUG);
    shaderLinearSampling_      = shaderCompiler.Compile(source, { { "USE_POINT_SAMPLING", "0" }, { "USE_GAMMA", "0" } }, RTRC_DEBUG);
    shaderPointSamplingGamma_  = shaderCompiler.Compile(source, { { "USE_POINT_SAMPLING", "1" }, { "USE_GAMMA", "1" } }, RTRC_DEBUG);
    shaderLinearSamplingGamma_ = shaderCompiler.Compile(source, { { "USE_POINT_SAMPLING", "0" }, { "USE_GAMMA", "1" } }, RTRC_DEBUG);
}

void CopyTextureUtils::RenderFullscreenTriangle(
    CommandBuffer    &commandBuffer,
    const TextureSrv &src,
    const TextureRtv &dst,
    SamplingMethod    samplingMethod,
    float             gamma)
{
    commandBuffer.BeginRenderPass(RHI::ColorAttachment
    {
        .renderTargetView = dst,
        .loadOp           = RHI::AttachmentLoadOp::DontCare,
        .storeOp          = RHI::AttachmentStoreOp::Store
    });
    RTRC_SCOPE_EXIT{ commandBuffer.EndRenderPass(); };

    const bool enableGamma = gamma != 1.0f;
    auto dstTexture = dst.GetTexture().get();
    auto pipeline = GetPipeline(Key{ dstTexture->GetFormat(), samplingMethod, enableGamma });
    commandBuffer.BindGraphicsPipeline(pipeline);

    RC<BindingGroup> passGroup;
    if(enableGamma)
    {
        CopyTextureUtilsDetail::Pass<true> passData;
        passData.Src = src;
        passData.gamma = gamma;
        passGroup = device_->CreateBindingGroupWithCachedLayout(passData);
    }
    else
    {
        CopyTextureUtilsDetail::Pass<false> passData;
        passData.Src = src;
        passGroup = device_->CreateBindingGroupWithCachedLayout(passData);
    }
    commandBuffer.BindGraphicsGroup(0, passGroup);

    commandBuffer.SetViewports(dstTexture->GetViewport());
    commandBuffer.SetScissors(dstTexture->GetScissor());
    commandBuffer.Draw(3, 1, 0, 0);
}

const RC<GraphicsPipeline> &CopyTextureUtils::GetPipeline(const Key &key)
{
    {
        std::shared_lock lock(mutex_);
        if(auto it = keyToPipeline_.find(key); it != keyToPipeline_.end())
        {
            return it->second;
        }
    }

    std::lock_guard lock(mutex_);
    if(auto it = keyToPipeline_.find(key); it != keyToPipeline_.end())
    {
        return it->second;
    }

    auto &shader = key.samplingMethod == Point ?
        (key.enableGamma ? shaderPointSamplingGamma_ : shaderPointSampling_) :
        (key.enableGamma ? shaderLinearSamplingGamma_ : shaderLinearSampling_);
    auto newPipeline = device_->CreateGraphicsPipeline(GraphicsPipeline::Desc
    {
        .shader                 = shader,
        .colorAttachmentFormats = { key.dstFormat }
    });
    auto it = keyToPipeline_.insert({ key, std::move(newPipeline) }).first;
    return it->second;
}

RTRC_END
