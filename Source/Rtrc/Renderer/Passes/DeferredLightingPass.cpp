#include <Rtrc/Renderer/GBufferBinding.h>
#include <Rtrc/Renderer/Passes/DeferredLightingPass.h>
#include <Rtrc/Renderer/Scene/RenderCamera.h>
#include <Rtrc/Renderer/Utility/FullscreenPrimitive.h>
#include <Rtrc/Utility/Enumerate.h>

RTRC_RENDERER_BEGIN

namespace DeferredLightingPassDetail
{

    template<DeferredLightingPass::MainLightMode mode>
    using SelectMainLightShadingData =
        std::conditional_t<
            mode == DeferredLightingPass::MainLightMode_None, BindingGroupDSL::RtrcGroupDummyMemberType,
        std::conditional_t<
            mode == DeferredLightingPass::MainLightMode_Point, PointLightShadingData,
                                                               DirectionalLightShadingData>>;

    template<DeferredLightingPass::MainLightMode Mode>
    rtrc_group(BindingGroup_Lighting, CS)
    {
        rtrc_inline(GBufferBindings_All, gbuffers);

        rtrc_define(ConstantBuffer<CameraConstantBuffer>, camera);

        rtrc_define(StructuredBuffer, pointLightBuffer);
        rtrc_uniform(uint,            pointLightCount);

        rtrc_define(StructuredBuffer, directionalLightBuffer);
        rtrc_uniform(uint,            directionalLightCount);
        
        using MainLightShadingData = SelectMainLightShadingData<Mode>;
        rtrc_define(Texture2D,             MainLightShadowMask);
        rtrc_uniform(MainLightShadingData, mainLightShadingData);

        rtrc_define(RWTexture2D, Output);
        rtrc_uniform(uint2,  resolution);
        rtrc_uniform(float2, rcpResolution);
    };

    rtrc_group(BindingGroup_Sky, CS)
    {
        rtrc_inline(GBufferBindings_Depth, gbuffers);
        rtrc_define(ConstantBuffer<CameraConstantBuffer>, camera);
        rtrc_define(Texture2D, SkyLut);
        rtrc_define(RWTexture2D, Output);
        rtrc_uniform(uint2, resolution);
        rtrc_uniform(float2, rcpResolution);
    };

} // namespace DeferredLightingPassDetail

DeferredLightingPass::DeferredLightingPass(
    ObserverPtr<Device> device, ObserverPtr<const BuiltinResourceManager> builtinResources)
    : device_(device)
    , builtinResources_(builtinResources)
    , lightBufferPool_(device, RHI::BufferUsage::ShaderStructuredBuffer)
{
    auto lightingMaterial = builtinResources_->GetBuiltinMaterial(BuiltinMaterial::DeferredLighting);
    auto regularShaderTemplate = lightingMaterial->GetPassByIndex(0)->GetShaderTemplate().get();
    regularShader_NoMainLight_ = regularShaderTemplate->GetShader(
        { { RTRC_KEYWORD(MAIN_LIGHT_MODE), MainLightMode_None } });
    regularShader_DirectionalMainLight_ = regularShaderTemplate->GetShader(
        { { RTRC_KEYWORD(MAIN_LIGHT_MODE), MainLightMode_Directional } });
    regularShader_PointMainLight_ = regularShaderTemplate->GetShader(
        { { RTRC_KEYWORD(MAIN_LIGHT_MODE), MainLightMode_Point } });
    skyShader_ = lightingMaterial->GetPassByIndex(1)->GetShader();
    shadowMaskPass_ = MakeBox<ShadowMaskPass>(device_, builtinResources_);
}

void DeferredLightingPass::Render(
    const RenderCamera  &scene,
    const GBuffers      &gbuffers,
    RG::TextureResource *skyLut,
    RG::RenderGraph     &renderGraph,
    RG::TextureResource *renderTarget)
{
    RG::TextureResource *shadowMask = nullptr;
    auto &renderLights = scene.GetScene().GetRenderLights();
    if(renderLights.HasMainLight())
    {
        shadowMask = shadowMaskPass_->Render(scene, renderLights.GetMainLight(), gbuffers, renderGraph);
    }

    auto RenderImpl = [&]<MainLightMode mainLightMode>()
    {
        using PassGroup = DeferredLightingPassDetail::BindingGroup_Lighting<mainLightMode>;
        auto lightingPass = renderGraph.CreatePass("DeferredLighting");
        lightingPass->Use(renderTarget, RG::CS_RWTexture_WriteOnly);
        lightingPass->Use(renderLights.GetRGPointLightBuffer(), RG::CS_StructuredBuffer);
        lightingPass->Use(renderLights.GetRGDirectionalLightBuffer(), RG::CS_StructuredBuffer);
        DeclareGBufferUses<PassGroup>(lightingPass, gbuffers, RHI::PipelineStage::ComputeShader);
        if(skyLut)
        {
            lightingPass->Use(skyLut, RG::PS_Texture);
        }
        if(shadowMask)
        {
            lightingPass->Use(shadowMask, RG::PS_Texture);
        }
        lightingPass->SetCallback(
            [this, gbuffers, shadowMask, skyLut, renderTarget, &scene]
            (RG::PassContext &context)
        {
            DoDeferredLighting<mainLightMode>(scene, gbuffers, skyLut, shadowMask, renderTarget, context);
        });
    };

    if(renderLights.HasMainLight())
    {
        auto mainLight = renderLights.GetMainLight();
        if(mainLight->GetType() == Light::Type::Point)
        {
            RenderImpl.operator()<MainLightMode_Point>();
            return;
        }
        if(mainLight->GetType() == Light::Type::Directional)
        {
            RenderImpl.operator()<MainLightMode_Directional>();
            return;
        }
        LogError("Unsupported main light type: {}", static_cast<int>(mainLight->GetType()));
    }
    return RenderImpl.operator()<MainLightMode_None>();
}

template<DeferredLightingPass::MainLightMode Mode>
void DeferredLightingPass::DoDeferredLighting(
    const RenderCamera &sceneCamera,
    const GBuffers          &gbuffers,
    RG::TextureResource     *skyLut,
    RG::TextureResource     *shadowMask,
    RG::TextureResource     *rgRenderTarget,
    RG::PassContext         &context)
{
    using namespace DeferredLightingPassDetail;

    auto &renderLights          = sceneCamera.GetScene().GetRenderLights();
    auto pointLightBuffer       = renderLights.GetRGPointLightBuffer();
    auto directionalLightBuffer = renderLights.GetRGDirectionalLightBuffer();

    // Per-pass binding group

    BindingGroup_Lighting<Mode> lightingPassData;
    FillBindingGroupGBuffers(lightingPassData, gbuffers, context);
    lightingPassData.camera                 = sceneCamera.GetCameraCBuffer();
    lightingPassData.pointLightBuffer       = pointLightBuffer->Get(context)->GetStructuredSrv(sizeof(PointLightShadingData));
    lightingPassData.pointLightCount        = renderLights.GetPointLights().size();
    lightingPassData.directionalLightBuffer = directionalLightBuffer->Get(context)->GetStructuredSrv(sizeof(DirectionalLightShadingData));
    lightingPassData.directionalLightCount  = renderLights.GetDirectionalLights().size();
    if constexpr(Mode == MainLightMode_Point)
    {
        lightingPassData.mainLightShadingData = renderLights.GetMainPointLightShadingData();
    }
    else if constexpr(Mode == MainLightMode_Directional)
    {
        lightingPassData.mainLightShadingData = renderLights.GetMainDirectionalLightShadingData();
    }
    else
    {
        static_assert(Mode == MainLightMode_None);
    }
    lightingPassData.MainLightShadowMask = shadowMask ? shadowMask->Get(context)
                                                      : builtinResources_->GetBuiltinTexture(BuiltinTexture::White2D);
    lightingPassData.Output = rgRenderTarget->Get(context);
    lightingPassData.resolution = rgRenderTarget->GetSize();
    lightingPassData.rcpResolution.x = 1.0f / lightingPassData.resolution.x;
    lightingPassData.rcpResolution.y = 1.0f / lightingPassData.resolution.y;
    auto lightingPassGroup = device_->CreateBindingGroup(lightingPassData);

    BindingGroup_Sky skyPassData;
    FillBindingGroupGBuffers(skyPassData, gbuffers, context);
    skyPassData.camera = lightingPassData.camera;
    skyPassData.SkyLut = skyLut ? skyLut->Get(context) : builtinResources_->GetBuiltinTexture(BuiltinTexture::Black2D);
    skyPassData.Output = lightingPassData.Output;
    skyPassData.resolution = lightingPassData.resolution;
    skyPassData.rcpResolution = lightingPassData.rcpResolution;
    auto skyPassGroup = device_->CreateBindingGroup(skyPassData);

    // Render
    
    for(const LightingPass lightingPass : { LightingPass::Regular, LightingPass::Sky })
    {
        Shader *shader;
        if(lightingPass == LightingPass::Regular)
        {
            if constexpr(Mode == MainLightMode_None)
            {
                shader = regularShader_NoMainLight_.get();
            }
            else if constexpr(Mode == MainLightMode_Point)
            {
                shader = regularShader_PointMainLight_.get();
            }
            else
            {
                static_assert(Mode == MainLightMode_Directional);
                shader = regularShader_DirectionalMainLight_.get();
            }
        }
        else
        {
            shader = skyShader_.get();
        }
        
        CommandBuffer &commandBuffer = context.GetCommandBuffer();
        commandBuffer.BindComputePipeline(shader->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, lightingPass == LightingPass::Regular ? lightingPassGroup : skyPassGroup);
        commandBuffer.DispatchWithThreadCount(rgRenderTarget->GetWidth(), rgRenderTarget->GetHeight());
    }
}

RTRC_RENDERER_END
