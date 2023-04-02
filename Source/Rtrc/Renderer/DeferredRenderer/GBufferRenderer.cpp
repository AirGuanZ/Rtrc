#include <Rtrc/Renderer/DeferredRenderer/GBufferRenderer.h>
#include <Rtrc/Utility/Enumerate.h>

RTRC_RENDERER_BEGIN

GBufferRenderer::GBufferRenderer(Device &device)
    : device_(device)
{
    
}

GBufferRenderer::RenderGraphInterface GBufferRenderer::AddToRenderGraph(
    DeferredRendererCommon::FrameContext &frameContext,
    RG::RenderGraph                      &renderGraph,
    const Vector2u                       &rtSize)
{
    const GBuffers gbuffers = AllocateGBuffers(renderGraph, rtSize);

    auto pass = renderGraph.CreatePass("RenderGBuffers");
    pass->Use(gbuffers.a, RG::COLOR_ATTACHMENT_WRITEONLY);
    pass->Use(gbuffers.b, RG::COLOR_ATTACHMENT_WRITEONLY);
    pass->Use(gbuffers.c, RG::COLOR_ATTACHMENT_WRITEONLY);
    pass->Use(gbuffers.depth, RG::DEPTH_STENCIL_ATTACHMENT);
    pass->SetCallback([this, gbuffers, &frameContext](RG::PassContext &passContext)
    {
        DoRenderGBufferPass(passContext, frameContext, gbuffers);
    });

    RenderGraphInterface ret;
    ret.gbuffers = gbuffers;
    ret.gbufferPass = pass;
    return ret;
}

GBufferRenderer::GBuffers GBufferRenderer::AllocateGBuffers(RG::RenderGraph &renderGraph, const Vector2u &rtSize) const
{
    GBuffers ret;
    ret.a = renderGraph.CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::A2R10G10B10_UNorm,
        .width                = rtSize.x,
        .height               = rtSize.y,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    }, "GBufferA");
    ret.b = renderGraph.CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R8G8B8A8_UNorm,
        .width                = rtSize.x,
        .height               = rtSize.y,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    }, "GBufferB");
    ret.c = renderGraph.CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R8G8B8A8_UNorm,
        .width                = rtSize.x,
        .height               = rtSize.y,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    }, "GBufferC");
    ret.depth = renderGraph.CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::D24S8,
        .width                = rtSize.x,
        .height               = rtSize.y,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::DepthStencil | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    }, "GBufferDepth");
    return ret;
}

void GBufferRenderer::DoRenderGBufferPass(
    RG::PassContext                      &passContext,
    DeferredRendererCommon::FrameContext &frameContext,
    const GBuffers                       &gbuffers)
{
    using namespace DeferredRendererCommon;

    CommandBuffer &cmd = passContext.GetCommandBuffer();

    auto gbufferA = gbuffers.a->Get(passContext);
    auto gbufferB = gbuffers.b->Get(passContext);
    auto gbufferC = gbuffers.c->Get(passContext);
    auto gbufferDepth = gbuffers.depth->Get(passContext);
    
    // Render pass

    cmd.BeginRenderPass(
    {
        ColorAttachment
        {
            .renderTargetView = gbufferA->CreateRtv(),
            .loadOp           = AttachmentLoadOp::Clear,
            .storeOp          = AttachmentStoreOp::Store,
            .clearValue       = { 0, 0, 0, 0 }
        },
        ColorAttachment
        {
            .renderTargetView = gbufferB->CreateRtv(),
            .loadOp           = AttachmentLoadOp::Clear,
            .storeOp          = AttachmentStoreOp::Store,
            .clearValue       = { 0, 0, 0, 0 }
        },
        ColorAttachment
        {
            .renderTargetView = gbufferC->CreateRtv(),
            .loadOp           = AttachmentLoadOp::Clear,
            .storeOp          = AttachmentStoreOp::Store,
            .clearValue       = { 0, 0, 0, 0 }
        }
    },
    DepthStencilAttachment
    {
        .depthStencilView = gbufferDepth->CreateDsv(),
        .loadOp           = AttachmentLoadOp::Clear,
        .storeOp          = AttachmentStoreOp::Store,
        .clearValue       = DepthStencilClearValue{ 1, 0 }
    });
    RTRC_SCOPE_EXIT{ cmd.EndRenderPass(); };

    KeywordContext keywords;

    // Per pass binding groups

    RC<BindingGroup> perPassGroup;
    {
        GBuffer_Pass perPassGroupData;
        perPassGroupData.Camera = CameraConstantBuffer::FromCamera(*frameContext.camera);
        perPassGroup = device_.CreateBindingGroup(perPassGroupData);
    }

    // Filter & sort static mesh renderers

    std::vector<const StaticMeshRenderProxy *> staticMeshRendererProxies =
    {
        frameContext.scene->GetStaticMeshRenderObjects().begin(),
        frameContext.scene->GetStaticMeshRenderObjects().end(),
    };
    std::ranges::sort(staticMeshRendererProxies,
        [](const StaticMeshRenderProxy *lhs, const StaticMeshRenderProxy *rhs)
    {
        return std::make_tuple(lhs->materialRenderingData.Get(), lhs->meshRenderingData.Get()) <
               std::make_tuple(rhs->materialRenderingData.Get(), rhs->meshRenderingData.Get());
    });

    // Upload constant buffers for static meshes

    std::vector<RC<BindingGroup>> staticMeshBindingGroups;
    staticMeshBindingGroups.reserve(staticMeshRendererProxies.size());
    for(const StaticMeshRenderProxy *staticMesh : staticMeshRendererProxies)
    {
        StaticMeshCBuffer cbuffer;
        cbuffer.localToWorld  = staticMesh->localToWorld;
        cbuffer.worldToLocal  = staticMesh->worldToLocal;
        cbuffer.localToCamera = cbuffer.localToWorld * frameContext.camera->GetWorldToCameraMatrix();
        cbuffer.localToClip   = cbuffer.localToWorld * frameContext.camera->GetWorldToClipMatrix();

        auto bindingGroup = frameContext.transientConstantBufferAllocator->CreateConstantBufferBindingGroup(cbuffer);
        staticMeshBindingGroups.push_back(std::move(bindingGroup));
    }
    frameContext.transientConstantBufferAllocator->Flush();

    // Dynamic states

    cmd.SetStencilReferenceValue(std::to_underlying(StencilMaskBit::Regular));
    cmd.SetViewports(gbufferA->GetViewport());
    cmd.SetScissors(gbufferA->GetScissor());

    // Render static meshes

    const GraphicsPipeline *lastPipeline = nullptr;
    const Mesh::SharedRenderingData *lastMesh = nullptr;

    for(auto &&[staticMeshIndex, staticMesh] : Enumerate(staticMeshRendererProxies))
    {
        auto &mesh = staticMesh->meshRenderingData;
        auto &matInst = staticMesh->materialRenderingData;

        auto matPassInst = matInst->GetPassInstance("GBuffer");
        if(!matPassInst)
        {
            continue;
        }

        auto pass = matPassInst->GetPass();
        auto shader = matPassInst->GetShader(keywords);
        auto pipeline = renderGBuffersPipelines_.GetOrCreate(
            pass, shader->GetUniqueID(), mesh->GetLayout(), [&]
            {
                return device_.CreateGraphicsPipeline(
                {
                    .shader            = shader,
                    .meshLayout        = mesh->GetLayout(),
                    .primitiveTopology = RHI::PrimitiveTopology::TriangleList,
                    .fillMode          = RHI::FillMode::Fill,
                    .cullMode          = RHI::CullMode::CullBack,
                    .frontFaceMode     = RHI::FrontFaceMode::CounterClockwise,
                    .enableDepthTest   = true,
                    .enableDepthWrite  = true,
                    .depthCompareOp    = RHI::CompareOp::Less,
                    .enableStencilTest = true,
                    .stencilReadMask   = 0xff,
                    .stencilWriteMask  = std::to_underlying(StencilMaskBit::Regular),
                    .frontStencil      = GraphicsPipeline::StencilOps
                    {
                        .depthFailOp = RHI::StencilOp::Keep,
                        .failOp      = RHI::StencilOp::Keep,
                        .passOp      = RHI::StencilOp::Replace,
                        .compareOp   = RHI::CompareOp::Always
                    },
                    .colorAttachmentFormats =
                    {
                        gbuffers.a->GetFormat(),
                        gbuffers.b->GetFormat(),
                        gbuffers.c->GetFormat()
                    },
                    .depthStencilFormat = gbuffers.depth->GetFormat(),
                });
            });

        if(pipeline.get() != lastPipeline)
        {
            cmd.BindGraphicsPipeline(pipeline);
            lastPipeline = pipeline.get();
            lastMesh = nullptr;
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

        if(mesh.Get() != lastMesh)
        {
            mesh->BindVertexAndIndexBuffers(cmd);
            lastMesh = mesh.Get();
        }

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

RTRC_RENDERER_END
