#include <Rtrc/Graphics/Material/ShaderBindingContext.h>
#include <Rtrc/Renderer/Renderer.h>
#include <Rtrc/Renderer/Utility/FullscreenPrimitive.h>
#include <Rtrc/Renderer/Utility/ScopedGPUDebugEvent.h>
#include <Rtrc/Utility/Enumerate.h>

RTRC_BEGIN

namespace DeferredRendererDetail
{

    rtrc_group(GBuffer_Pass, Pass)
    {
        rtrc_uniform(CameraConstantBuffer, Camera);
    };

    rtrc_group(DeferredLighting_Pass, Pass)
    {
        rtrc_define(Texture2D<float4>, gbufferA,     FS);
        rtrc_define(Texture2D<float4>, gbufferB,     FS);
        rtrc_define(Texture2D<float4>, gbufferC,     FS);
        rtrc_define(Texture2D<float>,  gbufferDepth, FS);
        rtrc_define(Texture2D<float3>, skyLut,       FS);

        rtrc_uniform(Vector4f,                       gbufferSize);
        rtrc_uniform(CameraConstantBuffer,           camera);
        rtrc_uniform(DirectionalLightConstantBuffer, directionalLight);
    };

} // namespace DeferredRendererDetail

Renderer::Renderer(Device &device, BuiltinResourceManager &builtinResources)
    : device_(device)
    , builtinResources_(builtinResources)
    , staticMeshConstantBufferBatch_(
          ConstantBufferSize<StaticMeshCBuffer>, "Object", RHI::ShaderStageFlags::All, device)
{

}

Renderer::RenderGraphInterface Renderer::AddToRenderGraph(
    const Parameters    &parameters,
    RG::RenderGraph     *renderGraph,
    RG::TextureResource *renderTarget,
    const Scene         &scene,
    const Camera        &camera)
{
    staticMeshConstantBufferBatch_.NewBatch();

    scene_ = &scene;
    camera_ = &camera;

    const uint32_t rtWidth = renderTarget->GetWidth();
    const uint32_t rtHeight = renderTarget->GetHeight();

    fullscreenQuadWithRays_ = GetFullscreenQuad(device_, camera);
    fullscreenTriangleWithRays_ = GetFullscreenTriangle(device_, camera);

    // gbufferA: normal
    // gbufferB: albedo, metallic
    // gbufferC: roughness

    const auto gbufferA = renderGraph->CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R10G10B10A2_UNorm,
        .width                = rtWidth,
        .height               = rtHeight,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    }, "GBufferA");
    const auto gbufferB = renderGraph->CreateTexture(RHI::TextureDesc
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
    const auto gbufferC = renderGraph->CreateTexture(RHI::TextureDesc
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
    const auto gbufferDepth = renderGraph->CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::D32,
        .width                = rtWidth,
        .height               = rtHeight,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::DepthStencil | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    }, "GBufferDepth");

    const auto image = renderTarget;

    RG::Pass *gbufferPass = renderGraph->CreatePass("Render GBuffers");
    {
        gbufferPass->Use(gbufferA, RG::RENDER_TARGET_WRITE);
        gbufferPass->Use(gbufferB, RG::RENDER_TARGET_WRITE);
        gbufferPass->Use(gbufferC, RG::RENDER_TARGET_WRITE);
        gbufferPass->Use(gbufferDepth, RG::DEPTH_STENCIL);

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

    RG::Pass *lightingPass = renderGraph->CreatePass("Deferred Lighting");
    {
        lightingPass->Use(gbufferA, RG::PIXEL_SHADER_TEXTURE_READ);
        lightingPass->Use(gbufferB, RG::PIXEL_SHADER_TEXTURE_READ);
        lightingPass->Use(gbufferC, RG::PIXEL_SHADER_TEXTURE_READ);
        lightingPass->Use(gbufferDepth, RG::PIXEL_SHADER_TEXTURE_READ);
        lightingPass->Use(image, RG::RENDER_TARGET_WRITE);
        lightingPass->Use(parameters.skyLut, RG::PIXEL_SHADER_TEXTURE_READ);

        DeferredLightingPassData passData;
        passData.gbufferA     = gbufferA;
        passData.gbufferB     = gbufferB;
        passData.gbufferC     = gbufferC;
        passData.gbufferDepth = gbufferDepth;
        passData.image        = image;
        passData.skyLut       = parameters.skyLut;

        lightingPass->SetCallback([this, passData](RG::PassContext &passContext)
        {
            DoDeferredLightingPass(passContext, passData);
        });
    }

    Connect(gbufferPass, lightingPass);

    RenderGraphInterface ret;
    ret.inPass = gbufferPass;
    ret.outPass = lightingPass;
    return ret;
}

void Renderer::DoRenderGBuffersPass(RG::PassContext &passContext, const RenderGBuffersPassData &passData)
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
            .renderTargetView = gbufferA->CreateRTV(),
            .loadOp = AttachmentLoadOp::Clear,
            .storeOp = AttachmentStoreOp::Store,
            .clearValue = { 0, 0, 0, 0 }
        },
        ColorAttachment
        {
            .renderTargetView = gbufferB->CreateRTV(),
            .loadOp = AttachmentLoadOp::Clear,
            .storeOp = AttachmentStoreOp::Store,
            .clearValue = { 0, 0, 0, 0 }
        },
        ColorAttachment
        {
            .renderTargetView = gbufferC->CreateRTV(),
            .loadOp = AttachmentLoadOp::Clear,
            .storeOp = AttachmentStoreOp::Store,
            .clearValue = { 0, 0, 0, 0 }
        }
    },
    DepthStencilAttachment
    {
        .depthStencilView = gbufferDepth->CreateDSV(),
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
        perPassGroupData.Camera = camera_->GetConstantBufferData();
        perPassGroup = device_.CreateBindingGroup(perPassGroupData);
    }

    // Upload constant buffers for static meshes

    std::vector<RC<BindingGroup>> staticMeshBindingGroups;
    staticMeshBindingGroups.reserve(scene_->GetAllStaticMeshes().size());
    for(auto &staticMesh : scene_->GetAllStaticMeshes())
    {
        StaticMeshCBuffer cbuffer;
        cbuffer.localToWorld = staticMesh->GetLocalToWorld();
        cbuffer.worldToLocal = staticMesh->GetWorldToLocal();
        cbuffer.localToCamera = cbuffer.localToWorld * camera_->GetWorldToCameraMatrix();
        cbuffer.localToClip = cbuffer.localToWorld * camera_->GetWorldToClipMatrix();
        staticMeshBindingGroups.push_back(staticMeshConstantBufferBatch_.NewRecord(cbuffer).bindingGroup);
    }
    staticMeshConstantBufferBatch_.Flush();

    // Render static meshes

    RC<GraphicsPipeline> lastPipeline;
    for(auto &&[staticMeshIndex, staticMesh] : Enumerate(scene_->GetAllStaticMeshes()))
    {
        auto &mesh = staticMesh->GetMesh();
        auto &matInst = staticMesh->GetMaterial();

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
            cmd.BindPipeline(pipeline);
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
        cmd.SetViewports(gbufferA->GetViewport());
        cmd.SetScissors(gbufferA->GetScissor());
        cmd.BindMesh(*mesh);

        if(mesh->HasIndexBuffer())
        {
            cmd.DrawIndexed(mesh->GetIndexCount(), 1, 0, 0, 0);
        }
        else
        {
            cmd.Draw(mesh->GetVertexCount(), 1, 0, 0);
        }
    }
}

void Renderer::DoDeferredLightingPass(RG::PassContext &passContext, const DeferredLightingPassData &passData)
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

    cmd.BeginRenderPass(ColorAttachment
    {
        .renderTargetView = image->CreateRTV(),
        .loadOp = AttachmentLoadOp::DontCare,
        .storeOp = AttachmentStoreOp::Store,
    });
    RTRC_SCOPE_EXIT{ cmd.EndRenderPass(); };

    DeferredRendererDetail::DeferredLighting_Pass perPassGroupData;
    perPassGroupData.gbufferA = gbufferA;
    perPassGroupData.gbufferB = gbufferB;
    perPassGroupData.gbufferC = gbufferC;
    perPassGroupData.gbufferDepth = gbufferDepth;
    perPassGroupData.skyLut = skyLut;
    perPassGroupData.camera = camera_->GetConstantBufferData();
    perPassGroupData.gbufferSize =
        Vector4f(image->GetWidth(), image->GetHeight(), 1.0f / image->GetWidth(), 1.0f / image->GetHeight());
    if(auto &dl = scene_->GetDirectionalLight())
    {
        const Light::DirectionalData &data = dl->GetDirectionalData();
        perPassGroupData.directionalLight.color = dl->GetColor();
        perPassGroupData.directionalLight.intensity = dl->GetIntensity();
        perPassGroupData.directionalLight.direction = data.direction;
    }
    else
    {
        perPassGroupData.directionalLight.intensity = 0;
    }
    
    cmd.BindPipeline(deferredLightingPipeline_);

    RC<BindingGroup> perPassGroup = device_.CreateBindingGroup(perPassGroupData);
    const int groupIndex =
        deferredLightingPipeline_->GetShaderInfo()->GetBuiltinBindingGroupIndex(Shader::BuiltinBindingGroup::Pass);
    cmd.BindGraphicsGroup(groupIndex, std::move(perPassGroup));

    if(auto &inlineSamplerGroup = deferredLightingPipeline_->GetShaderInfo()->GetBindingGroupForInlineSamplers())
    {
        cmd.BindGraphicsGroup(
            deferredLightingPipeline_->GetShaderInfo()->GetBindingGroupIndexForInlineSamplers(), inlineSamplerGroup);
    }

    cmd.SetViewports(gbufferA->GetViewport());
    cmd.SetScissors(gbufferA->GetScissor());
    cmd.BindMesh(fullscreenTriangleWithRays_);
    cmd.Draw(fullscreenTriangleWithRays_.GetVertexCount(), 1, 0, 0);
}

RTRC_END
