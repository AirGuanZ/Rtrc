#include <Rtrc/Renderer/GBufferBinding.h>
#include <Rtrc/Renderer/Passes/DeferredLightingPass.h>
#include <Rtrc/Renderer/Utility/FullscreenPrimitive.h>

RTRC_RENDERER_BEGIN

namespace DeferredLightingPassDetail
{
    
    rtrc_group(BindingGroup_DeferredLightingPass, FS)
    {
        rtrc_inline(GBufferBindings_All, gbuffers);

        rtrc_define(ConstantBuffer<CameraConstantBuffer>, camera);

        rtrc_define(StructuredBuffer, pointLightBuffer);
        rtrc_uniform(uint,            pointLightCount);

        rtrc_define(StructuredBuffer, directionalLightBuffer);
        rtrc_uniform(uint,            directionalLightCount);

        rtrc_define(Texture2D, SkyLut);
    };

} // namespace DeferredLightingPassDetail

DeferredLightingPass::DeferredLightingPass(
    ObserverPtr<Device> device, ObserverPtr<const BuiltinResourceManager> builtinResources)
    : device_(device)
    , builtinResources_(builtinResources)
    , pipelineCache_(device)
    , lightBufferPool_(device, RHI::BufferUsage::ShaderStructuredBuffer)
{
    perPassBindingGroupLayout_ =
        device_->CreateBindingGroupLayout<DeferredLightingPassDetail::BindingGroup_DeferredLightingPass>();

    auto lightingMaterial = builtinResources_->GetBuiltinMaterial(BuiltinMaterial::DeferredLighting);
    regularShaderTemplate_ = lightingMaterial->GetPassByIndex(0)->GetShaderTemplate();
    skyShaderTemplate_     = lightingMaterial->GetPassByIndex(1)->GetShaderTemplate();

    pipelineTemplate_ = GraphicsPipeline::Desc
    {
        .meshLayout = GetFullscreenPrimitiveMeshLayoutWithWorldRay(),
        .enableStencilTest = true,
        .stencilReadMask = 0xff,
        .frontStencil = GraphicsPipeline::StencilOps
        {
            .depthFailOp = RHI::StencilOp::Keep,
            .failOp      = RHI::StencilOp::Keep,
            .passOp      = RHI::StencilOp::Keep,
            .compareOp   = RHI::CompareOp::NotEqual // Shaded pixel has non-zero stencil value
        }
    };
}

DeferredLightingPass::RenderGraphOutput DeferredLightingPass::RenderDeferredLighting(
    const CachedScenePerCamera &scene,
    RG::RenderGraph            &renderGraph,
    const GBuffers             &gbuffers,
    RG::TextureResource        *skyLut,
    RG::TextureResource        *renderTarget)
{
    RenderGraphOutput ret;

    ret.lightingPass = renderGraph.CreatePass("DeferredLighting");
    DeclareGBufferUses<true, DeferredLightingPassDetail::BindingGroup_DeferredLightingPass>(
        ret.lightingPass, gbuffers, RHI::PipelineStage::FragmentShader);
    ret.lightingPass->Use(renderTarget, RG::COLOR_ATTACHMENT_WRITEONLY);
    if(skyLut)
    {
        ret.lightingPass->Use(skyLut, RG::PIXEL_SHADER_TEXTURE);
    }

    ret.lightingPass->SetCallback([this, gbuffers, skyLut, renderTarget, &scene](RG::PassContext &context)
    {
        DoDeferredLighting(scene, gbuffers, skyLut, renderTarget, context);
    });

    return ret;
}

void DeferredLightingPass::DoDeferredLighting(
    const CachedScenePerCamera &scene,
    const GBuffers             &rgGBuffers,
    RG::TextureResource        *skyLut,
    RG::TextureResource        *rgRenderTarget,
    RG::PassContext            &context)
{
    using namespace DeferredLightingPassDetail;

    // Collect lights

    std::vector<PointLightShadingData>       pointLightData;
    std::vector<DirectionalLightShadingData> directionalLightData;

    for(const Light::SharedRenderingData *light : scene.GetLights())
    {
        if(light->GetType() == Light::Type::Point)
        {
            auto &data = pointLightData.emplace_back();
            data.position = light->GetPosition();
            data.color = light->GetIntensity() * light->GetColor();

            // distFade = saturate((distEnd - dist) / (distEnd - distBegin))
            //          = saturate(-dist/(distEnd-distBegin) + distEnd/(distEnd-distBegin))
            const float deltaDist = std::max(light->GetDistanceFadeEnd() - light->GetDistanceFadeBegin(), 1e-5f);
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

    RC<Texture> gbufferA = rgGBuffers.normal->Get(context);
    RC<Texture> gbufferB = rgGBuffers.albedoMetallic->Get(context);
    RC<Texture> gbufferC = rgGBuffers.roughness->Get(context);
    RC<Texture> gbufferDepth = rgGBuffers.depth->Get(context);

    BindingGroup_DeferredLightingPass passData;
    FillBindingGroupGBuffers<true>(passData, rgGBuffers, context);
    passData.camera                 = scene.GetCameraCBuffer();
    passData.pointLightBuffer       = pointLightBuffer->GetStructuredSrv(sizeof(PointLightShadingData));
    passData.pointLightCount        = static_cast<uint32_t>(pointLightData.size());
    passData.directionalLightBuffer = directionalLightBuffer->GetStructuredSrv(sizeof(DirectionalLightShadingData));
    passData.directionalLightCount  = static_cast<uint32_t>(directionalLightData.size());

    if(skyLut)
    {
        passData.SkyLut = skyLut->Get(context);
    }
    else
    {
        passData.SkyLut = builtinResources_->GetBuiltinTexture(BuiltinTexture::Black2D);
    }

    auto passBindingGroup = device_->CreateBindingGroup(passData, perPassBindingGroupLayout_);
    
    // Render Pass

    RC<Texture> renderTarget = rgRenderTarget->Get(context);

    CommandBuffer &commandBuffer = context.GetCommandBuffer();
    commandBuffer.BeginRenderPass(
        ColorAttachment
        {
            .renderTargetView = renderTarget->CreateRtv(),
            .loadOp           = AttachmentLoadOp::DontCare,
            .storeOp          = AttachmentStoreOp::Store
        },
        DepthStencilAttachment
        {
            .depthStencilView = gbufferDepth->CreateDsv(
                                    RHI::TextureViewFlagBit::DepthSrv_StencilAttachmentReadOnly),
            .loadOp           = AttachmentLoadOp::Load,
            .storeOp          = AttachmentStoreOp::Store
        });
    RTRC_SCOPE_EXIT{ commandBuffer.EndRenderPass(); };

    // Mesh

    const Mesh mesh = GetFullscreenTriangle(*device_, scene.GetCamera());
    BindMesh(commandBuffer, mesh);

    for(const LightingPass lightingPass : { LightingPass::Regular, LightingPass::Sky })
    {
        // Pipeline

        GraphicsPipeline::Desc pipelineDesc = pipelineTemplate_;
        RC<Shader> shader;
        if(lightingPass == LightingPass::Regular)
        {
            shader = regularShaderTemplate_->GetShader();
        }
        else
        {
            assert(lightingPass == LightingPass::Sky);
            shader = skyShaderTemplate_->GetShader();
            pipelineDesc.frontStencil.compareOp = RHI::CompareOp::Equal;
        }

        regularShaderTemplate_->GetShader();
        pipelineDesc.shader = shader;
        pipelineDesc.colorAttachmentFormats = { renderTarget->GetFormat() };
        pipelineDesc.depthStencilFormat = rgGBuffers.depth->GetFormat();

        RC<GraphicsPipeline> pipeline = pipelineCache_.GetGraphicsPipeline(pipelineDesc);
        commandBuffer.BindGraphicsPipeline(pipeline);

        // Dynamic states

        commandBuffer.SetStencilReferenceValue(std::to_underlying(StencilBit::None));
        commandBuffer.SetViewports(renderTarget->GetViewport());
        commandBuffer.SetScissors(renderTarget->GetScissor());

        commandBuffer.BindGraphicsGroup(
            shader->GetBuiltinBindingGroupIndex(ShaderBindingLayoutInfo::BuiltinBindingGroup::Pass),
            passBindingGroup);

        // Render

        commandBuffer.Draw(mesh.GetVertexCount(), 1, 0, 0);
    }

    pipelineCache_.CommitChanges();
}

RTRC_RENDERER_END
