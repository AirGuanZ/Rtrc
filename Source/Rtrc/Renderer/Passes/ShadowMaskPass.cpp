#include <Rtrc/Renderer/GBufferBinding.h>
#include <Rtrc/Renderer/Passes/ShadowMaskPass.h>

RTRC_RENDERER_BEGIN

namespace ShadowMaskPassDetail
{

    rtrc_group(BindingGroup_ShadowMaskPass, CS)
    {
        rtrc_inline(GBufferBindings_NormalDepth, gbuffers);

        rtrc_define(RaytracingAccelerationStructure, Scene);
        rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera);

        rtrc_define(RWTexture2D, OutputTextureRW);

        rtrc_uniform(uint2,  outputResolution);
        rtrc_uniform(float2, rcpOutputResolution);
        rtrc_uniform(uint,   lightType);
        rtrc_uniform(float3, lightGeometry);
    };

} // namespace ShadowMaskPassDetail

ShadowMaskPass::ShadowMaskPass(
    ObserverPtr<Device>                       device,
    ObserverPtr<const BuiltinResourceManager> builtinResources)
    : device_(device), builtinResources_(builtinResources)
{
    shader_ = builtinResources->GetBuiltinMaterial(BuiltinMaterial::ShadowMask)
                              ->GetPassByTag("BruteForce")->GetShader();
    passBindingGroupLayout_ = device_->CreateBindingGroupLayout<ShadowMaskPassDetail::BindingGroup_ShadowMaskPass>();
}

ShadowMaskPass::RenderGraphOutput ShadowMaskPass::RenderShadowMask(
    const CachedScenePerCamera &scene,
    int                         lightIndex,
    const RGScene              &rgScene,
    RG::RenderGraph            &renderGraph)
{
    if(!device_->IsRayTracingEnabled() || lightIndex < 0)
    {
        return {};
    }

    const Light::SharedRenderingData *light = scene.GetLights()[lightIndex];
    if(!light->GetFlags().Contains(Light::Flags::EnableRayTracedShadow))
    {
        return {};
    }

    const Vector2u outputSize = rgScene.gbuffers.normal->GetSize();
    RG::TextureResource *shadowMask = renderGraph.CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R8_UNorm,
        .width                = outputSize.x,
        .height               = outputSize.y,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::UnorderAccess | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    }, "ShadowMask");

    auto pass = renderGraph.CreatePass("ShadowMask");
    DeclareGBufferUses<false, ShadowMaskPassDetail::BindingGroup_ShadowMaskPass>(
        pass, rgScene.gbuffers, RHI::PipelineStage::ComputeShader);
    pass->Use(rgScene.opaqueTlasBuffer, RG::UseInfo
    {
        .layout   = RHI::TextureLayout::Undefined,
        .stages   = RHI::PipelineStage::ComputeShader,
        .accesses = RHI::ResourceAccess::ReadAS
    });
    pass->Use(shadowMask, RG::COMPUTE_SHADER_RWTEXTURE_WRITEONLY);
    pass->SetCallback(
        [gbuffers = rgScene.gbuffers, &scene, shadowMask, outputSize, light, this]
        (RG::PassContext &context)
    {
        ShadowMaskPassDetail::BindingGroup_ShadowMaskPass passData;
        FillBindingGroupGBuffers<false>(passData, gbuffers, context);
        passData.Scene               = scene.GetCachedScene().GetOpaqueTlas();
        passData.Camera              = scene.GetCameraCBuffer();
        passData.OutputTextureRW     = shadowMask->Get(context);
        passData.outputResolution    = outputSize;
        passData.rcpOutputResolution = Vector2f(1.0f / outputSize.x, 1.0f / outputSize.y);
        passData.lightType           = light->GetType() == Light::Type::Point ? 0 : 1;
        passData.lightGeometry       = light->GetType() == Light::Type::Point ? light->GetPosition() : light->GetDirection();
        auto passGroup = device_->CreateBindingGroup(passData, passBindingGroupLayout_);

        CommandBuffer &commandBuffer = context.GetCommandBuffer();
        commandBuffer.BindComputePipeline(shader_->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(outputSize.x, outputSize.y, 1);
    });

    RenderGraphOutput ret;
    ret.shadowMask = shadowMask;
    ret.shadowMaskPass = pass;
    return ret;
}

RTRC_RENDERER_END
