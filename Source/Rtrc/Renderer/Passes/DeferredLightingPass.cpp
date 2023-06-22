#include <Rtrc/Renderer/GBufferBinding.h>
#include <Rtrc/Renderer/Passes/DeferredLightingPass.h>
#include <Rtrc/Renderer/Scene/PersistentSceneCameraRenderingData.h>
#include <Rtrc/Renderer/Utility/FullscreenPrimitive.h>
#include <Rtrc/Utility/Enumerate.h>

RTRC_RENDERER_BEGIN

namespace DeferredLightingPassDetail
{
    
    rtrc_group(BindingGroup_DeferredLightingPass, CS)
    {
        rtrc_inline(GBufferBindings_All, gbuffers);

        rtrc_define(ConstantBuffer<CameraConstantBuffer>, camera);

        rtrc_define(StructuredBuffer, pointLightBuffer);
        rtrc_uniform(uint,            pointLightCount);

        rtrc_define(StructuredBuffer, directionalLightBuffer);
        rtrc_uniform(uint,            directionalLightCount);

        rtrc_define(Texture2D, SkyLut);

        rtrc_define(Texture2D, ShadowMask);
        rtrc_uniform(uint,     shadowMaskLightType); // 0: none; 1: point; 2: directional
                                                     // If >= 1, should be applied to the first of the specified type

        rtrc_define(RWTexture2D, Output);
        rtrc_uniform(uint2,  resolution);
        rtrc_uniform(float2, rcpResolution);
    };

} // namespace DeferredLightingPassDetail

DeferredLightingPass::DeferredLightingPass(
    ObserverPtr<Device> device, ObserverPtr<const BuiltinResourceManager> builtinResources)
    : device_(device)
    , builtinResources_(builtinResources)
    , lightBufferPool_(device, RHI::BufferUsage::ShaderStructuredBuffer)
{
    perPassBindingGroupLayout_ =
        device_->CreateBindingGroupLayout<DeferredLightingPassDetail::BindingGroup_DeferredLightingPass>();

    auto lightingMaterial = builtinResources_->GetBuiltinMaterial(BuiltinMaterial::DeferredLighting);
    regularShader_ = lightingMaterial->GetPassByIndex(0)->GetShader();
    skyShader_ = lightingMaterial->GetPassByIndex(1)->GetShader();
}

DeferredLightingPass::RenderGraphOutput DeferredLightingPass::RenderDeferredLighting(
    const PersistentSceneCameraRenderingData &scene,
    const RGScene              &rgScene,
    RG::RenderGraph            &renderGraph,
    RG::TextureResource        *renderTarget)
{
    const GBuffers &gbuffers = rgScene.gbuffers;

    RenderGraphOutput ret;
    ret.lightingPass = renderGraph.CreatePass("DeferredLighting");
    DeclareGBufferUses<DeferredLightingPassDetail::BindingGroup_DeferredLightingPass>(
        ret.lightingPass, gbuffers, RHI::PipelineStage::ComputeShader);
    ret.lightingPass->Use(renderTarget, RG::CS_RWTexture_WriteOnly);
    if(rgScene.skyLut)
    {
        ret.lightingPass->Use(rgScene.skyLut, RG::PS_Texture);
    }
    if(rgScene.shadowMask)
    {
        ret.lightingPass->Use(rgScene.shadowMask, RG::PS_Texture);
    }

    ret.lightingPass->SetCallback(
        [this, &rgScene, renderTarget, &scene]
        (RG::PassContext &context)
    {
        DoDeferredLighting(scene, rgScene, renderTarget, context);
    });

    return ret;
}

void DeferredLightingPass::DoDeferredLighting(
    const PersistentSceneCameraRenderingData  &sceneCamera,
    const RGScene       &rgScene,
    RG::TextureResource *rgRenderTarget,
    RG::PassContext     &context)
{
    using namespace DeferredLightingPassDetail;

    // Collect lights

    std::vector<PointLightShadingData>       pointLightData;
    std::vector<DirectionalLightShadingData> directionalLightData;

    int shadowMaskLightType = 0;
    for(auto &&[lightIndex, light] : Enumerate(sceneCamera.GetLights()))
    {
        if(light->GetType() == Light::Type::Point)
        {
            auto &data = pointLightData.emplace_back();
            data.position = light->GetPosition();
            data.color = light->GetIntensity() * light->GetColor();

            // distFade = saturate((distEnd - dist) / (distEnd - distBegin))
            //          = saturate(-dist/(distEnd-distBegin) + distEnd/(distEnd-distBegin))
            const float deltaDist = (std::max)(light->GetDistanceFadeEnd() - light->GetDistanceFadeBegin(), 1e-5f);
            data.distFadeScale = -1.0f / deltaDist;
            data.distFadeBias = light->GetDistanceFadeEnd() / deltaDist;
        }
        else if(light->GetType() == Light::Type::Directional)
        {
            auto &data = directionalLightData.emplace_back();
            data.direction = light->GetDirection();
            data.color = light->GetIntensity() * light->GetColor();
        }
        else
        {
            LogWarning(
                "DeferredLightPass: unsupported light type ({}). Only point/directional lights are supported",
                std::to_underlying(light->GetType()));
        }

        if(static_cast<int>(lightIndex) == rgScene.shadowMaskLightIndex)
        {
            if(light->GetType() == Light::Type::Point)
            {
                std::swap(pointLightData[0], pointLightData.back());
                shadowMaskLightType = 1;
            }
            else if(light->GetType() == Light::Type::Directional)
            {
                std::swap(directionalLightData[0], directionalLightData.back());
                shadowMaskLightType = 2;
            }
            else
            {
                // Do not apply shadow mask for unsupported light type
            }
        }
    }

    auto pointLightBuffer = lightBufferPool_.Acquire(
        sizeof(PointLightShadingData) * std::max<size_t>(pointLightData.size(), 1));
    pointLightBuffer->Upload(
        pointLightData.data(), 0, sizeof(PointLightShadingData) * pointLightData.size());

    auto directionalLightBuffer = lightBufferPool_.Acquire(
        sizeof(DirectionalLightShadingData) * std::max<size_t>(directionalLightData.size(), 1));
    directionalLightBuffer->Upload(
        directionalLightData.data(), 0, sizeof(DirectionalLightShadingData) * directionalLightData.size());

    // Per-pass binding group

    BindingGroup_DeferredLightingPass passData;
    FillBindingGroupGBuffers(passData, rgScene.gbuffers, context);
    passData.camera                 = sceneCamera.GetCameraCBuffer();
    passData.pointLightBuffer       = pointLightBuffer->GetStructuredSrv(sizeof(PointLightShadingData));
    passData.pointLightCount        = static_cast<uint32_t>(pointLightData.size());
    passData.directionalLightBuffer = directionalLightBuffer->GetStructuredSrv(sizeof(DirectionalLightShadingData));
    passData.directionalLightCount  = static_cast<uint32_t>(directionalLightData.size());
    passData.shadowMaskLightType    = shadowMaskLightType;
    if(rgScene.shadowMask)
    {
        passData.ShadowMask = rgScene.shadowMask->Get(context);
    }
    else
    {
        passData.ShadowMask = builtinResources_->GetBuiltinTexture(BuiltinTexture::White2D);
    }
    
    passData.SkyLut = rgScene.skyLut ? rgScene.skyLut->Get(context)
                                     : builtinResources_->GetBuiltinTexture(BuiltinTexture::Black2D);
    passData.Output = rgRenderTarget->Get(context);
    passData.resolution = rgRenderTarget->GetSize();
    passData.rcpResolution.x = 1.0f / passData.resolution.x;
    passData.rcpResolution.y = 1.0f / passData.resolution.y;

    auto passBindingGroup = device_->CreateBindingGroup(passData, perPassBindingGroupLayout_);

    // Render
    
    for(const LightingPass lightingPass : { LightingPass::Regular, LightingPass::Sky })
    {
        auto shader = lightingPass == LightingPass::Regular ? regularShader_ : skyShader_;

        CommandBuffer &commandBuffer = context.GetCommandBuffer();
        commandBuffer.BindComputePipeline(shader->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, passBindingGroup);
        commandBuffer.DispatchWithThreadCount(passData.resolution.x, passData.resolution.y, 1);
    }
}

RTRC_RENDERER_END
