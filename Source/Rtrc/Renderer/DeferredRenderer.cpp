#include <Rtrc/Renderer/DeferredRenderer.h>
#include <Rtrc/Renderer/Utility/FullscreenPrimitive.h>
#include <Rtrc/Renderer/Utility/ScopedGPUDebugEvent.h>
#include <Rtrc/Utility/Enumerate.h>

RTRC_BEGIN

namespace DeferredRendererDetail
{

    // See Asset/Builtin/Material/Common/Scene.hlsl
    rtrc_struct(DirectionalLightConstantBuffer)
    {
        rtrc_var(float3, direction) = Vector3f(0, -1, 0);
        rtrc_var(float3, color) = Vector3f(1, 1, 1);
        rtrc_var(float, intensity) = 1;
    };

    // See Asset/Builtin/Material/Common/Scene.hlsl
    rtrc_struct(CameraConstantBuffer)
    {
        static CameraConstantBuffer FromCamera(const Camera &camera)
        {
            CameraConstantBuffer ret;
            ret.worldToCameraMatrix = camera.GetWorldToCameraMatrix();
            ret.cameraToWorldMatrix = camera.GetCameraToWorldMatrix();
            ret.cameraToClipMatrix = camera.GetCameraToClipMatrix();
            ret.clipToCameraMatrix = camera.GetClipToCameraMatrix();
            ret.worldToClipMatrix = camera.GetWorldToClipMatrix();
            ret.clipToWorldMatrix = camera.GetClipToWorldMatrix();
            ret.worldRays[0] = camera.GetWorldRays()[0];
            ret.worldRays[1] = camera.GetWorldRays()[1];
            ret.worldRays[2] = camera.GetWorldRays()[2];
            ret.worldRays[3] = camera.GetWorldRays()[3];
            ret.cameraRays[0] = camera.GetCameraRays()[0];
            ret.cameraRays[1] = camera.GetCameraRays()[1];
            ret.cameraRays[2] = camera.GetCameraRays()[2];
            ret.cameraRays[3] = camera.GetCameraRays()[3];
            return ret;
        }

        rtrc_var(float4x4, worldToCameraMatrix);
        rtrc_var(float4x4, cameraToWorldMatrix);
        rtrc_var(float4x4, cameraToClipMatrix);
        rtrc_var(float4x4, clipToCameraMatrix);
        rtrc_var(float4x4, worldToClipMatrix);
        rtrc_var(float4x4, clipToWorldMatrix);
        rtrc_var(float3[4], cameraRays);
        rtrc_var(float3[4], worldRays);
    };

    rtrc_group(GBuffer_Pass)
    {
        rtrc_uniform(CameraConstantBuffer, Camera);
    };

    rtrc_group(DeferredLighting_Pass, FS)
    {
        rtrc_define(Texture2D, gbufferA,     FS);
        rtrc_define(Texture2D, gbufferB,     FS);
        rtrc_define(Texture2D, gbufferC,     FS);
        rtrc_define(Texture2D, gbufferDepth, FS);
        rtrc_define(Texture2D, skyLut,       FS);

        rtrc_uniform(float4,                         gbufferSize);
        rtrc_uniform(CameraConstantBuffer,           camera);
        rtrc_uniform(DirectionalLightConstantBuffer, directionalLight);
    };

} // namespace DeferredRendererDetail

DeferredRenderer::DeferredRenderer(Device &device, const BuiltinResourceManager &builtinResources)
    : device_(device)
    , builtinResources_(builtinResources)
    , transientConstantBufferAllocator_(device)
{
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

    scene_ = &scene;
    camera_ = &camera;

    const uint32_t rtWidth = renderTarget->GetWidth();
    const uint32_t rtHeight = renderTarget->GetHeight();

    fullscreenQuadWithRays_ = GetFullscreenQuad(device_, camera);
    fullscreenTriangleWithRays_ = GetFullscreenTriangle(device_, camera);

    // gbufferA: normal
    // gbufferB: albedo, metallic
    // gbufferC: roughness

    const auto gbufferA = renderGraph.CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::A2R10G10B10_UNorm,
        .width                = rtWidth,
        .height               = rtHeight,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    }, "GBufferA");
    const auto gbufferB = renderGraph.CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R8G8B8A8_UNorm,
        .width                = rtWidth,
        .height               = rtHeight,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    }, "GBufferB");
    const auto gbufferC = renderGraph.CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R8G8B8A8_UNorm,
        .width                = rtWidth,
        .height               = rtHeight,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    }, "GBufferC");
    const auto gbufferDepth = renderGraph.CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::D24S8,
        .width                = rtWidth,
        .height               = rtHeight,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::DepthStencil | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    }, "GBufferDepth");

    auto rgAtmosphere = atmosphereRenderer_->AddToRenderGraph(renderGraph, camera, deltaTime);

    RG::Pass *gbufferPass = renderGraph.CreatePass("Render GBuffers");
    {
        gbufferPass->Use(gbufferA, RG::COLOR_ATTACHMENT_WRITEONLY);
        gbufferPass->Use(gbufferB, RG::COLOR_ATTACHMENT_WRITEONLY);
        gbufferPass->Use(gbufferC, RG::COLOR_ATTACHMENT_WRITEONLY);
        gbufferPass->Use(gbufferDepth, RG::DEPTH_STENCIL_ATTACHMENT);

        RenderGBuffersPassData passData;
        passData.gbufferA     = gbufferA;
        passData.gbufferB     = gbufferB;
        passData.gbufferC     = gbufferC;
        passData.gbufferDepth = gbufferDepth;

        gbufferPass->SetCallback([this, passData](RG::PassContext &passContext)
        {
            DoRenderGBuffersPass(passContext, passData);
        });
    }

    RG::Pass *lightingPass = renderGraph.CreatePass("Deferred Lighting");
    {
        lightingPass->Use(gbufferA, RG::PIXEL_SHADER_TEXTURE);
        lightingPass->Use(gbufferB, RG::PIXEL_SHADER_TEXTURE);
        lightingPass->Use(gbufferC, RG::PIXEL_SHADER_TEXTURE);
        lightingPass->Use(gbufferDepth, RG::UseInfo
        {
            .layout = RHI::TextureLayout::DepthShaderTexture_StencilReadOnlyAttachment,
            .stages = RHI::PipelineStage::FragmentShader | RHI::PipelineStage::DepthStencil,
            .accesses = RHI::ResourceAccess::TextureRead | RHI::ResourceAccess::DepthStencilRead
        });
        lightingPass->Use(renderTarget, RG::COLOR_ATTACHMENT_WRITEONLY);
        lightingPass->Use(rgAtmosphere.skyLut, RG::PIXEL_SHADER_TEXTURE);

        DeferredLightingPassData passData;
        passData.gbufferA     = gbufferA;
        passData.gbufferB     = gbufferB;
        passData.gbufferC     = gbufferC;
        passData.gbufferDepth = gbufferDepth;
        passData.image        = renderTarget;
        passData.skyLut       = rgAtmosphere.skyLut;

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

void DeferredRenderer::DoRenderGBuffersPass(RG::PassContext &passContext, const RenderGBuffersPassData &passData)
{
    CommandBuffer &cmd = passContext.GetCommandBuffer();

    auto gbufferA = passData.gbufferA->Get();
    auto gbufferB = passData.gbufferB->Get();
    auto gbufferC = passData.gbufferC->Get();
    auto gbufferDepth = passData.gbufferDepth->Get();

    gbufferA->SetName("GBufferA");
    gbufferB->SetName("GBufferB");
    gbufferC->SetName("GBufferC");
    gbufferDepth->SetName("GBufferDepth");
    
    // Render pass

    cmd.BeginRenderPass(
    {
        ColorAttachment
        {
            .renderTargetView = gbufferA->CreateRtv(),
            .loadOp = AttachmentLoadOp::Clear,
            .storeOp = AttachmentStoreOp::Store,
            .clearValue = { 0, 0, 0, 0 }
        },
        ColorAttachment
        {
            .renderTargetView = gbufferB->CreateRtv(),
            .loadOp = AttachmentLoadOp::Clear,
            .storeOp = AttachmentStoreOp::Store,
            .clearValue = { 0, 0, 0, 0 }
        },
        ColorAttachment
        {
            .renderTargetView = gbufferC->CreateRtv(),
            .loadOp = AttachmentLoadOp::Clear,
            .storeOp = AttachmentStoreOp::Store,
            .clearValue = { 0, 0, 0, 0 }
        }
    },
    DepthStencilAttachment
    {
        .depthStencilView = gbufferDepth->CreateDsv(),
        .loadOp = AttachmentLoadOp::Clear,
        .storeOp = AttachmentStoreOp::Store,
        .clearValue = DepthStencilClearValue{ 1, 0 }
    });
    RTRC_SCOPE_EXIT{ cmd.EndRenderPass(); };

    KeywordValueContext keywords;

    // Per pass binding groups

    RC<BindingGroup> perPassGroup;
    {
        DeferredRendererDetail::GBuffer_Pass perPassGroupData;
        perPassGroupData.Camera = DeferredRendererDetail::CameraConstantBuffer::FromCamera(*camera_);
        perPassGroup = device_.CreateBindingGroup(perPassGroupData);
    }

    // Upload constant buffers for static meshes

    std::vector<const StaticMeshRendererProxy *> staticMeshRendererProxies;
    for(const RendererProxy *proxy : scene_->GetRenderers())
    {
        if(proxy->type == RendererProxy::Type::StaticMesh)
        {
            staticMeshRendererProxies.push_back(static_cast<const StaticMeshRendererProxy *>(proxy));
        }
    }

    std::vector<RC<BindingGroup>> staticMeshBindingGroups;
    staticMeshBindingGroups.reserve(staticMeshRendererProxies.size());
    for(const StaticMeshRendererProxy *staticMesh : staticMeshRendererProxies)
    {
        StaticMeshCBuffer cbuffer;
        cbuffer.localToWorld  = staticMesh->localToWorld;
        cbuffer.worldToLocal  = staticMesh->worldToLocal;
        cbuffer.localToCamera = cbuffer.localToWorld * camera_->GetWorldToCameraMatrix();
        cbuffer.localToClip   = cbuffer.localToWorld * camera_->GetWorldToClipMatrix();

        auto bindingGroup = transientConstantBufferAllocator_.CreateConstantBufferBindingGroup(cbuffer);
        staticMeshBindingGroups.push_back(std::move(bindingGroup));
    }
    transientConstantBufferAllocator_.Flush();

    // Dynamic states

    cmd.SetStencilReferenceValue(std::to_underlying(StencilMaskBit::Regular));
    cmd.SetViewports(gbufferA->GetViewport());
    cmd.SetScissors(gbufferA->GetScissor());

    // Render static meshes

    RC<GraphicsPipeline> lastPipeline;
    for(auto &&[staticMeshIndex, staticMesh] : Enumerate(staticMeshRendererProxies))
    {
        auto &mesh = staticMesh->meshRenderingData;
        auto &matInst = staticMesh->materialRenderingData;

        auto matPassInst = matInst->GetPassInstance("GBuffer");
        if(!matPassInst)
            continue;

        auto pass = matPassInst->GetPass();
        auto shader = matPassInst->GetShader(keywords);
        auto pipeline = renderGBuffersPipelines_.GetOrCreate(
            pass, shader, mesh->GetLayout(), [&]
            {
                return device_.CreateGraphicsPipeline(
                {
                    .shader                 = shader,
                    .meshLayout             = mesh->GetLayout(),
                    .primitiveTopology      = RHI::PrimitiveTopology::TriangleList,
                    .fillMode               = RHI::FillMode::Fill,
                    .cullMode               = RHI::CullMode::CullBack,
                    .frontFaceMode          = RHI::FrontFaceMode::CounterClockwise,
                    .enableDepthTest        = true,
                    .enableDepthWrite       = true,
                    .depthCompareOp         = RHI::CompareOp::Less,
                    .enableStencilTest      = true,
                    .stencilReadMask        = 0xff,
                    .stencilWriteMask       = std::to_underlying(StencilMaskBit::Regular),
                    .frontStencil           = GraphicsPipeline::StencilOps
                    {
                        .depthFailOp = RHI::StencilOp::Keep,
                        .failOp      = RHI::StencilOp::Keep,
                        .passOp      = RHI::StencilOp::Replace,
                        .compareOp   = RHI::CompareOp::Always
                    },
                    .colorAttachmentFormats =
                    {
                        passData.gbufferA->GetFormat(),
                        passData.gbufferB->GetFormat(),
                        passData.gbufferC->GetFormat()
                    },
                    .depthStencilFormat     = passData.gbufferDepth->GetFormat(),
                });
            });

        if(pipeline != lastPipeline)
        {
            cmd.BindGraphicsPipeline(pipeline);
            lastPipeline = pipeline;
        }

        BindMaterialProperties(*matPassInst, keywords, cmd, true);
        if(int index = shader->GetBuiltinBindingGroupIndex(ShaderInfo::BuiltinBindingGroup::Pass); index >= 0)
        {
            cmd.BindGraphicsGroup(index, perPassGroup);
        }
        if(int index = shader->GetBuiltinBindingGroupIndex(ShaderInfo::BuiltinBindingGroup::Object); index >= 0)
        {
            cmd.BindGraphicsGroup(index, staticMeshBindingGroups[staticMeshIndex]);
        }
        mesh->Bind(cmd);

        if(mesh->GetIndexCount() > 0)
        {
            cmd.DrawIndexed(mesh->GetIndexCount(), 1, 0, 0, 0);
        }
        else
        {
            cmd.Draw(mesh->GetVertexCount(), 1, 0, 0);
        }
    }
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

    auto gbufferA = passData.gbufferA->Get();
    auto gbufferB = passData.gbufferB->Get();
    auto gbufferC = passData.gbufferC->Get();
    auto gbufferDepth = passData.gbufferDepth->Get();
    auto image = passData.image->Get();
    auto skyLut = passData.skyLut->Get();

    {
        cmd.BeginRenderPass(ColorAttachment
            {
                .renderTargetView = image->CreateRtv(),
                .loadOp = AttachmentLoadOp::DontCare,
                .storeOp = AttachmentStoreOp::Store,
            });
        RTRC_SCOPE_EXIT{ cmd.EndRenderPass(); };

        DeferredRendererDetail::DeferredLighting_Pass perPassGroupData;
        perPassGroupData.gbufferA = gbufferA;
        perPassGroupData.gbufferB = gbufferB;
        perPassGroupData.gbufferC = gbufferC;
        perPassGroupData.gbufferDepth = gbufferDepth->CreateSrv(
            RHI::TextureSrvFlagBit::SpecialLayout_DepthSrv_StencilAttachmentReadOnly);
        perPassGroupData.skyLut = skyLut;
        perPassGroupData.camera = DeferredRendererDetail::CameraConstantBuffer::FromCamera(*camera_);
        perPassGroupData.gbufferSize = Vector4f(
            image->GetWidth(), image->GetHeight(),
            1.0f / image->GetWidth(), 1.0f / image->GetHeight());

        const Light::SharedRenderingData *directionalLight = nullptr;
        for(const Light::SharedRenderingData *light : scene_->GetLights())
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

RTRC_END
