#include <Rtrc/Renderer/DeferredLighting/ShadowMaskPass.h>
#include <Rtrc/Renderer/GBufferBinding.h>
#include <Rtrc/Renderer/Scene/RenderCamera.h>
#include <Rtrc/Math/DistributionTransform.h>

RTRC_RENDERER_BEGIN

namespace ShadowMaskPassDetail
{
    
    rtrc_group(BindingGroup_CollectLowResShadowMaskPass, CS)
    {
        rtrc_inline(GBufferBindings_NormalDepth, gbuffers);

        rtrc_define(RaytracingAccelerationStructure, Scene);
        rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera);

        rtrc_define(RWTexture2D, OutputTextureRW);

        rtrc_define(Texture2D, BlueNoise256);
        rtrc_define(Texture2D, LowResMask);

        rtrc_uniform(uint2,  outputResolution);
        rtrc_uniform(float2, rcpOutputResolution);
        rtrc_uniform(uint,   lightType);
        rtrc_uniform(float3, lightGeometry);
        rtrc_uniform(float,  shadowSoftness);
    };

    rtrc_group(BindingGroup_BlurLowResShadowMaskPass, CS)
    {
        rtrc_define(Texture2D, In);
        rtrc_define(RWTexture2D, Out);
        rtrc_uniform(uint2, resolution);
    };

    rtrc_group(BindingGroup_ShadowMaskPass, CS)
    {
        rtrc_inline(GBufferBindings_NormalDepth, gbuffers);

        rtrc_define(RaytracingAccelerationStructure, Scene);
        rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera);

        rtrc_define(RWTexture2D, OutputTextureRW);

        rtrc_define(Texture2D, BlueNoise256);
        rtrc_define(Texture2D, LowResMask);
        
        rtrc_uniform(uint2,  outputResolution);
        rtrc_uniform(float2, rcpOutputResolution);
        rtrc_uniform(uint,   lightType);
        rtrc_uniform(float3, lightGeometry);
        rtrc_uniform(float,  shadowSoftness);
    };

    rtrc_group(BindingGroup_BlurPass, CS)
    {
        rtrc_inline(GBufferBindings_Depth, gbuffers);

        rtrc_define(Texture2D,   InShadowMask);
        rtrc_define(RWTexture2D, OutShadowMask);

        rtrc_uniform(uint2, resolution);
        rtrc_uniform(float4x4, cameraToClip);
    };

} // namespace ShadowMaskPassDetail

ShadowMaskPass::ShadowMaskPass(
    ObserverPtr<Device>                 device,
    ObserverPtr<BuiltinResourceManager> builtinResources)
    : device_(device), builtinResources_(builtinResources)
{
    InitializeShaders();
}

RG::TextureResource *ShadowMaskPass::Render(
    const RenderCamera               &camera,
    const Light::SharedRenderingData *light,
    const GBuffers                   &gbuffers,
    RG::RenderGraph                  &renderGraph)
{
    using namespace ShadowMaskPassDetail;

    if(!device_->IsRayTracingEnabled())
    {
        return {};
    }
    
    if(!light->GetFlags().Contains(Light::Flags::EnableRayTracedShadow))
    {
        return {};
    }

    RTRC_RG_SCOPED_PASS_GROUP(renderGraph, "ShadowMask");

    const Vector2u outputSize = gbuffers.normal->GetSize();
    const Vector2u lowResSize = Vector2u(
        std::max(1u, outputSize.x / 8),
        std::max(1u, outputSize.y / 8));

    RG::TextureResource *lowResShadowMask0 = nullptr;
    if(light->GetShadowSoftness() > 0)
    {
        lowResShadowMask0 = renderGraph.CreateTexture(RHI::TextureDesc
        {
            .dim                  = RHI::TextureDimension::Tex2D,
            .format               = RHI::Format::R8_UNorm,
            .width                = lowResSize.x,
            .height               = lowResSize.y,
            .depth                = 1,
            .mipLevels            = 1,
            .sampleCount          = 1,
            .usage                = RHI::TextureUsage::UnorderAccess | RHI::TextureUsage::ShaderResource,
            .initialLayout        = RHI::TextureLayout::Undefined,
            .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
        }, "LowResShadowMask0");
        auto generateLowResMaskPass = renderGraph.CreatePass("LowResShadowMask");
        DeclareGBufferUses<BindingGroup_CollectLowResShadowMaskPass>(
                generateLowResMaskPass, gbuffers, RHI::PipelineStage::ComputeShader);
        generateLowResMaskPass->Use(camera.GetScene().GetRGTlas(), RG::CS_ReadAS);
        generateLowResMaskPass->Use(lowResShadowMask0, RG::CS_RWTexture_WriteOnly);
        generateLowResMaskPass->SetCallback(
            [gbuffers, &camera, lowResShadowMask0, lowResSize, light, this]
        {
            BindingGroup_CollectLowResShadowMaskPass passData;
            FillBindingGroupGBuffers(passData, gbuffers);
            passData.Scene               = camera.GetScene().GetRGTlas();
            passData.Camera              = camera.GetCameraCBuffer();
            passData.OutputTextureRW     = lowResShadowMask0;
            passData.BlueNoise256        = builtinResources_->GetBuiltinTexture(BuiltinTexture::BlueNoise256);
            passData.LowResMask          = builtinResources_->GetBuiltinTexture(BuiltinTexture::Black2D);
            passData.outputResolution    = lowResSize;
            passData.rcpOutputResolution = Vector2f(1.0f / lowResSize.x, 1.0f / lowResSize.y);
            passData.lightType           = light->GetType() == Light::Type::Point ? 0 : 1;
            passData.lightGeometry       = light->GetType() == Light::Type::Point ? light->GetPosition() : light->GetDirection();
            passData.shadowSoftness      = light->GetShadowSoftness();

            auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);
            
            CommandBuffer &commandBuffer = RG::GetCurrentCommandBuffer();
            commandBuffer.BindComputePipeline(collectLowResShadowMaskShader_->GetComputePipeline());
            commandBuffer.BindComputeGroup(0, passGroup);
            commandBuffer.DispatchWithThreadCount(lowResSize.x, lowResSize.y);
        });

        RG::TextureResource *lowResShadowMask1 =
            renderGraph.CreateTexture(lowResShadowMask0->GetDesc(), "LowResShadowMask1");

        auto blurLowResShadowMaskPass0 = renderGraph.CreatePass("BlurLowResShadowMaskX");
        blurLowResShadowMaskPass0->Use(lowResShadowMask0, RG::CS_Texture);
        blurLowResShadowMaskPass0->Use(lowResShadowMask1, RG::CS_RWTexture_WriteOnly);
        blurLowResShadowMaskPass0->SetCallback(
            [lowResShadowMask0, lowResShadowMask1, lowResSize, this]
        {
            BindingGroup_BlurLowResShadowMaskPass passData;
            passData.In = lowResShadowMask0;
            passData.Out = lowResShadowMask1;
            passData.resolution = lowResSize;

            auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

            CommandBuffer &commandBuffer = RG::GetCurrentCommandBuffer();
            commandBuffer.BindComputePipeline(blurLowResXShader_->GetComputePipeline());
            commandBuffer.BindComputeGroup(0, passGroup);
            commandBuffer.DispatchWithThreadCount(lowResSize.x, lowResSize.y);
        });
        
        auto blurLowResShadowMaskPass1 = renderGraph.CreatePass("BlurLowResShadowMaskY");
        blurLowResShadowMaskPass1->Use(lowResShadowMask1, RG::CS_Texture);
        blurLowResShadowMaskPass1->Use(lowResShadowMask0, RG::CS_RWTexture_WriteOnly);
        blurLowResShadowMaskPass1->SetCallback(
            [lowResShadowMask0, lowResShadowMask1, lowResSize, this]
        {
            BindingGroup_BlurLowResShadowMaskPass passData;
            passData.In = lowResShadowMask1;
            passData.Out = lowResShadowMask0;
            passData.resolution = lowResSize;
            
            auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

            CommandBuffer &commandBuffer = RG::GetCurrentCommandBuffer();
            commandBuffer.BindComputePipeline(blurLowResYShader_->GetComputePipeline());
            commandBuffer.BindComputeGroup(0, passGroup);
            commandBuffer.DispatchWithThreadCount(lowResSize.x, lowResSize.y);
        });
    }

    RG::TextureResource *shadowMask0 = renderGraph.CreateTexture(RHI::TextureDesc
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

    RC<Shader> shadowMaskShader;
    {
        KeywordContext keywords;
        keywords.Set(RTRC_KEYWORD(ENABLE_SHADOW_SOFTNESS), light->GetShadowSoftness() > 0);
        keywords.Set(RTRC_KEYWORD(ENABLE_LOW_RES_MASK), light->GetShadowSoftness() > 0);
        shadowMaskShader = builtinResources_->GetBuiltinMaterial(BuiltinMaterial::ShadowMask)
                                            ->GetPassByTag(RTRC_MATERIAL_PASS_TAG(BruteForce))
                                            ->GetShaderTemplate()->GetShader(keywords);
    }

    auto generateRawMaskPass = renderGraph.CreatePass("ShadowMask");
    DeclareGBufferUses<BindingGroup_ShadowMaskPass>(generateRawMaskPass, gbuffers, RHI::PipelineStage::ComputeShader);
    generateRawMaskPass->Use(camera.GetScene().GetRGTlas(), RG::CS_ReadAS);
    if(lowResShadowMask0)
    {
        generateRawMaskPass->Use(lowResShadowMask0, RG::CS_Texture);
    }
    generateRawMaskPass->Use(shadowMask0, RG::CS_RWTexture_WriteOnly);
    generateRawMaskPass->SetCallback(
        [shadowMaskShader, gbuffers, &camera,
         lowResShadowMask0, shadowMask0, outputSize, light, this]
    {
        BindingGroup_ShadowMaskPass passData;
        FillBindingGroupGBuffers(passData, gbuffers);
        passData.Scene               = camera.GetScene().GetRGTlas();
        passData.Camera              = camera.GetCameraCBuffer();
        passData.OutputTextureRW     = shadowMask0;
        passData.BlueNoise256        = builtinResources_->GetBuiltinTexture(BuiltinTexture::BlueNoise256);
        passData.LowResMask          = lowResShadowMask0 ? lowResShadowMask0->Get()
                                                         : builtinResources_->GetBuiltinTexture(BuiltinTexture::Black2D);
        passData.outputResolution    = outputSize;
        passData.rcpOutputResolution = Vector2f(1.0f / outputSize.x, 1.0f / outputSize.y);
        passData.lightType           = light->GetType() == Light::Type::Point ? 0 : 1;
        passData.lightGeometry       = light->GetType() == Light::Type::Point ? light->GetPosition() : light->GetDirection();
        passData.shadowSoftness      = light->GetShadowSoftness();
        auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);
        
        CommandBuffer &commandBuffer = RG::GetCurrentCommandBuffer();
        commandBuffer.BindComputePipeline(shadowMaskShader->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(outputSize.x, outputSize.y);
    });

    if(light->GetShadowSoftness() <= 0)
    {
        return shadowMask0;
    }

    auto shadowMask1 = renderGraph.CreateTexture(shadowMask0->GetDesc(), "IntermediatelyBlurredShadowMask");

    auto blurXPass = renderGraph.CreatePass("BlurShadowMaskX");
    DeclareGBufferUses<BindingGroup_BlurPass>(blurXPass, gbuffers, RHI::PipelineStage::ComputeShader);
    blurXPass->Use(shadowMask0, RG::CS_Texture);
    blurXPass->Use(shadowMask1, RG::CS_RWTexture);
    blurXPass->SetCallback(
        [&camera, gbuffers, shadowMask0, shadowMask1, this, outputSize]
    {
        BindingGroup_BlurPass passData;
        FillBindingGroupGBuffers(passData, gbuffers);
        passData.InShadowMask = shadowMask0;
        passData.OutShadowMask = shadowMask1;
        passData.resolution = outputSize;
        passData.cameraToClip = camera.GetCameraRenderData().cameraToClip;
        auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

        CommandBuffer &commandBuffer = RG::GetCurrentCommandBuffer();
        commandBuffer.BindComputePipeline(blurXShader_->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(outputSize.x, outputSize.y);
    });
    
    auto blurYPass = renderGraph.CreatePass("BlurShadowMaskY");
    DeclareGBufferUses<BindingGroup_BlurPass>(blurYPass, gbuffers, RHI::PipelineStage::ComputeShader);
    blurYPass->Use(shadowMask1, RG::CS_Texture);
    blurYPass->Use(shadowMask0, RG::CS_RWTexture);
    blurYPass->SetCallback(
        [&camera, gbuffers, shadowMask0, shadowMask1, this, outputSize]
    {
        BindingGroup_BlurPass passData;
        FillBindingGroupGBuffers(passData, gbuffers);
        passData.InShadowMask = shadowMask1;
        passData.OutShadowMask = shadowMask0;
        passData.resolution = outputSize;
        passData.cameraToClip = camera.GetCameraRenderData().cameraToClip;
        auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

        CommandBuffer &commandBuffer = RG::GetCurrentCommandBuffer();
        commandBuffer.BindComputePipeline(blurYShader_->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(outputSize.x, outputSize.y);
    });
    
    return shadowMask0;
}

void ShadowMaskPass::InitializeShaders()
{
    Material *material = builtinResources_->GetBuiltinMaterial(BuiltinMaterial::ShadowMask).get();

    collectLowResShadowMaskShader_ = material->GetPassByTag(RTRC_MATERIAL_PASS_TAG(Collect))->GetShader();

    {
        auto shaderTemplate = material->GetPassByTag(RTRC_MATERIAL_PASS_TAG(CollectBlur))->GetShaderTemplate();
        blurLowResXShader_= shaderTemplate->GetShader(KeywordContext(RTRC_KEYWORD(IS_BLUR_DIRECTION_Y), 0));
        blurLowResYShader_ = shaderTemplate->GetShader(KeywordContext(RTRC_KEYWORD(IS_BLUR_DIRECTION_Y), 1));
    }

    {
        auto blurShaderTemplate = material->GetPassByTag(RTRC_MATERIAL_PASS_TAG(Blur))->GetShaderTemplate();
        blurXShader_ = blurShaderTemplate->GetShader(KeywordContext(RTRC_KEYWORD(IS_BLUR_DIRECTION_Y), 0));
        blurYShader_ = blurShaderTemplate->GetShader(KeywordContext(RTRC_KEYWORD(IS_BLUR_DIRECTION_Y), 1));
    }
}

RTRC_RENDERER_END