#include <ranges>

#include <Rtrc/Renderer/DeferredRenderer/DeferredRenderer.h>
#include <Rtrc/Renderer/Utility/FullscreenPrimitive.h>
#include <Rtrc/Renderer/Utility/ScopedGPUDebugEvent.h>
#include <Rtrc/Utility/Enumerate.h>

RTRC_RENDERER_BEGIN

DeferredRenderer::DeferredRenderer(Device &device, const BuiltinResourceManager &builtinResources)
    : device_(device)
    , builtinResources_(builtinResources)
    , transientConstantBufferAllocator_(device)
{
    gbufferRenderer_ = MakeBox<GBufferRenderer>(device);
    atmosphereRenderer_ = MakeBox<AtmosphereRenderer>(builtinResources);
}

AtmosphereRenderer &DeferredRenderer::GetAtmosphereRenderer()
{
    return *atmosphereRenderer_;
}

DeferredRenderer::RenderGraphInterface DeferredRenderer::AddToRenderGraph(
    RG::RenderGraph     &renderGraph,
    RG::TextureResource *renderTarget,
    const SceneProxy    &scene,
    const Camera        &camera,
    float                deltaTime)
{
    transientConstantBufferAllocator_.NewBatch();

    frameContext_.transientConstantBufferAllocator = &transientConstantBufferAllocator_;
    frameContext_.scene = &scene;
    frameContext_.camera = &camera;
    
    fullscreenQuadWithRays_ = GetFullscreenQuad(device_, camera);
    fullscreenTriangleWithRays_ = GetFullscreenTriangle(device_, camera);

    // GBuffers

    const auto rgGBuffers = gbufferRenderer_->AddToRenderGraph(
        frameContext_, renderGraph, { renderTarget->GetWidth(), renderTarget->GetHeight() });

    const GBufferRenderer::GBuffers gbuffers = rgGBuffers.gbuffers;
    RG::Pass *gbufferPass = rgGBuffers.gbufferPass;

    // Atmosphere

    const auto rgAtmosphere = atmosphereRenderer_->AddToRenderGraph(renderGraph, camera, deltaTime);

    // Lighting

    RG::Pass *lightingPass = renderGraph.CreatePass("Deferred Lighting");
    {
        lightingPass->Use(gbuffers.a, RG::PIXEL_SHADER_TEXTURE);
        lightingPass->Use(gbuffers.b, RG::PIXEL_SHADER_TEXTURE);
        lightingPass->Use(gbuffers.c, RG::PIXEL_SHADER_TEXTURE);
        lightingPass->Use(gbuffers.depth, RG::UseInfo
        {
            .layout = RHI::TextureLayout::DepthShaderTexture_StencilReadOnlyAttachment,
            .stages = RHI::PipelineStage::FragmentShader | RHI::PipelineStage::DepthStencil,
            .accesses = RHI::ResourceAccess::TextureRead | RHI::ResourceAccess::DepthStencilRead
        });
        lightingPass->Use(renderTarget, RG::COLOR_ATTACHMENT_WRITEONLY);
        lightingPass->Use(rgAtmosphere.skyLut, RG::PIXEL_SHADER_TEXTURE);

        DeferredLightingPassData passData;
        passData.gbuffers = gbuffers;
        passData.image    = renderTarget;
        passData.skyLut   = rgAtmosphere.skyLut;

        lightingPass->SetCallback([this, passData](RG::PassContext &passContext)
        {
            DoDeferredLightingPass(passContext, passData);
        });
    }

    Connect(rgAtmosphere.passOut, lightingPass);
    Connect(gbufferPass, lightingPass);

    RenderGraphInterface ret;
    ret.passIn = gbufferPass;
    ret.passOut = lightingPass;
    return ret;
}

void DeferredRenderer::DoDeferredLightingPass(RG::PassContext &passContext, const DeferredLightingPassData &passData)
{
    CommandBuffer &cmd = passContext.GetCommandBuffer();

    if(!deferredLightingPipeline_)
    {
        RC<Shader> shader = builtinResources_.GetBuiltinMaterial(BuiltinMaterial::DeferredLighting)
                                            ->GetPassByIndex(0)->GetShader();
        deferredLightingPipeline_ = device_.CreateGraphicsPipeline(GraphicsPipeline::Desc
        {
            .shader = std::move(shader),
            .meshLayout = fullscreenTriangleWithRays_.GetLayout(),
            .colorAttachmentFormats = { passData.image->GetFormat() }
        });
    }

    auto gbufferA = passData.gbuffers.a->Get(passContext);
    auto gbufferB = passData.gbuffers.b->Get(passContext);
    auto gbufferC = passData.gbuffers.c->Get(passContext);
    auto gbufferDepth = passData.gbuffers.depth->Get(passContext);
    auto image = passData.image->Get(passContext);
    auto skyLut = passData.skyLut->Get(passContext);

    {
        cmd.BeginRenderPass(ColorAttachment
            {
                .renderTargetView = image->CreateRtv(),
                .loadOp = AttachmentLoadOp::DontCare,
                .storeOp = AttachmentStoreOp::Store,
            });
        RTRC_SCOPE_EXIT{ cmd.EndRenderPass(); };

        DeferredRendererCommon::DeferredLighting_Pass perPassGroupData;
        perPassGroupData.gbufferA = gbufferA;
        perPassGroupData.gbufferB = gbufferB;
        perPassGroupData.gbufferC = gbufferC;
        perPassGroupData.gbufferDepth = gbufferDepth->CreateSrv(
            RHI::TextureSrvFlagBit::SpecialLayout_DepthSrv_StencilAttachmentReadOnly);
        perPassGroupData.skyLut = skyLut;
        perPassGroupData.camera = DeferredRendererCommon::CameraConstantBuffer::FromCamera(*frameContext_.camera);
        perPassGroupData.gbufferSize = Vector4f(
            image->GetWidth(), image->GetHeight(),
            1.0f / image->GetWidth(), 1.0f / image->GetHeight());

        const Light::SharedRenderingData *directionalLight = nullptr;
        for(const Light::SharedRenderingData *light : frameContext_.scene->GetLights())
        {
            if(light->GetType() == Light::Type::Directional)
            {
                directionalLight = light;
                break;
            }
        }
        if(directionalLight)
        {
            perPassGroupData.directionalLight.direction = directionalLight->GetDirection();
            perPassGroupData.directionalLight.color = directionalLight->GetColor();
            perPassGroupData.directionalLight.intensity = directionalLight->GetIntensity();
        }
        else
        {
            perPassGroupData.directionalLight.intensity = 0;
        }

        cmd.BindGraphicsPipeline(deferredLightingPipeline_);

        RC<BindingGroup> perPassGroup = device_.CreateBindingGroup(perPassGroupData);
        const int groupIndex =
            deferredLightingPipeline_->GetShaderInfo()->GetBuiltinBindingGroupIndex(Shader::BuiltinBindingGroup::Pass);
        cmd.BindGraphicsGroup(groupIndex, std::move(perPassGroup));

        cmd.SetViewports(gbufferA->GetViewport());
        cmd.SetScissors(gbufferA->GetScissor());
        cmd.BindMesh(fullscreenTriangleWithRays_);
        cmd.Draw(fullscreenTriangleWithRays_.GetVertexCount(), 1, 0, 0);
    }
}

RTRC_RENDERER_END
