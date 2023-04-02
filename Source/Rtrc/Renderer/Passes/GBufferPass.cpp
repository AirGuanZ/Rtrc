#include <Rtrc/Renderer/Common.h>
#include <Rtrc/Renderer/Passes/GBufferPass.h>
#include <Rtrc/Utility/Enumerate.h>

RTRC_RENDERER_BEGIN

namespace GBufferPassDetail
{

    rtrc_group(BindingGroup_GBufferPass)
    {
        rtrc_define(ConstantBuffer<CameraConstantBuffer>, camera);
    };

} // namespace GBufferPassDetail

GBufferPass::GBufferPass(ObserverPtr<Device> device)
    : device_(device), gbufferPipelineCache_(device)
{
    perPassBindingGroupLayout_ = device_->CreateBindingGroupLayout<GBufferPassDetail::BindingGroup_GBufferPass>();
}

GBufferPass::RenderGraphInterface GBufferPass::RenderGBuffers(
    const CachedScenePerCamera &scene,
    RG::RenderGraph            &renderGraph,
    const Vector2u             &rtSize)
{
    RenderGraphInterface ret;
    ret.gbuffers = AllocateGBuffers(renderGraph, rtSize);
    ret.gbufferPass = renderGraph.CreatePass("Render GBuffers");
    ret.gbufferPass->Use(ret.gbuffers.a, RG::COLOR_ATTACHMENT_WRITEONLY);
    ret.gbufferPass->Use(ret.gbuffers.b, RG::COLOR_ATTACHMENT_WRITEONLY);
    ret.gbufferPass->Use(ret.gbuffers.c, RG::COLOR_ATTACHMENT_WRITEONLY);
    ret.gbufferPass->Use(ret.gbuffers.depth, RG::DEPTH_STENCIL_ATTACHMENT);
    ret.gbufferPass->SetCallback([this, gbuffers = ret.gbuffers, &scene](RG::PassContext &context)
    {
        DoRenderGBuffers(context, scene, gbuffers);
    });
    return ret;
}

GBufferPass::GBuffers GBufferPass::AllocateGBuffers(RG::RenderGraph &renderGraph, const Vector2u &rtSize)
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

void GBufferPass::DoRenderGBuffers(
    RG::PassContext            &passContext,
    const CachedScenePerCamera &scene,
    const GBuffers             &gbuffers)
{
    auto gbufferA = gbuffers.a->Get(passContext);
    auto gbufferB = gbuffers.b->Get(passContext);
    auto gbufferC = gbuffers.c->Get(passContext);
    auto gbufferDepth = gbuffers.depth->Get(passContext);

    // Render Pass

    CommandBuffer &commandBuffer = passContext.GetCommandBuffer();
    commandBuffer.BeginRenderPass(
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
            .clearValue       = { 1, 0 }
        });
    RTRC_SCOPE_EXIT{ commandBuffer.EndRenderPass(); };
    
    // Per-pass binding group

    RC<BindingGroup> perPassBindingGroup;
    {
        GBufferPassDetail::BindingGroup_GBufferPass passData;
        passData.camera = scene.GetCameraCBuffer();
        perPassBindingGroup = device_->CreateBindingGroup(passData, perPassBindingGroupLayout_);
    }

    // Sort static meshes by material

    std::vector<const CachedScenePerCamera::StaticMeshRecord *> meshes;
    std::ranges::copy_if(
        scene.GetStaticMeshes(), std::back_inserter(meshes),
        [](const CachedScenePerCamera::StaticMeshRecord *mesh)
        {
            return mesh->gbufferPassIndex >= 0;
        });
    std::ranges::sort(meshes, [](const auto &a, const auto &b)
    {
        return std::make_tuple(a->cachedMaterial, a->cachedMesh) < std::make_tuple(a->cachedMaterial, b->cachedMesh);
    });

    // Keywords

    KeywordContext keywords;

    // Collect pipeline objects

    struct RenderOperation
    {
        RC<GraphicsPipeline>  pipeline;
        MaterialPassInstance *materialPassInstance;
        const Shader         *shader;
    };

    std::vector<RenderOperation> operations(meshes.size());
    {
        const MaterialInstance::SharedRenderingData *lastMaterial = nullptr;
        for(auto &&[index, record] : Enumerate(meshes))
        {
            const MaterialInstance::SharedRenderingData *materialInst = record->cachedMaterial->material;
            MaterialPassInstance *matPassInst = materialInst->GetPassInstance(record->gbufferPassIndex);
            operations[index].materialPassInstance = matPassInst;
            operations[index].shader = matPassInst->GetShader(keywords).get();
            if(materialInst != lastMaterial)
            {
                const RC<Shader> shader = matPassInst->GetShader(keywords);
                RC<GraphicsPipeline> pipeline = gbufferPipelineCache_.GetGraphicsPipeline(
                {
                    .shader            = shader,
                    .meshLayout        = record->cachedMesh->mesh->GetLayout(),
                    .primitiveTopology = RHI::PrimitiveTopology::TriangleList,
                    .fillMode          = RHI::FillMode::Fill,
                    .cullMode          = RHI::CullMode::CullBack,
                    .frontFaceMode     = RHI::FrontFaceMode::CounterClockwise,
                    .enableDepthTest   = true,
                    .enableDepthWrite  = true,
                    .depthCompareOp    = RHI::CompareOp::Less,
                    .enableStencilTest = true,
                    .stencilReadMask   = 0xff,
                    .stencilWriteMask  = std::to_underlying(StencilBit::Regular),
                    .frontStencil      = GraphicsPipeline::StencilOps
                    {
                        .depthFailOp = RHI::StencilOp::Keep,
                        .failOp      = RHI::StencilOp::Keep,
                        .passOp      = RHI::StencilOp::Replace,
                        .compareOp   = RHI::CompareOp::Always
                    },
                    .colorAttachmentFormats =
                    {
                        gbufferA->GetFormat(),
                        gbufferB->GetFormat(),
                        gbufferC->GetFormat()
                    },
                    .depthStencilFormat = gbuffers.depth->GetFormat()
                });

                operations[index].pipeline = std::move(pipeline);
                lastMaterial = materialInst;
            }
        }
    }
    gbufferPipelineCache_.CommitChanges();

    // Dynamic pipeline states

    commandBuffer.SetStencilReferenceValue(std::to_underlying(StencilBit::Regular));
    commandBuffer.SetViewports(gbufferA->GetViewport());
    commandBuffer.SetScissors(gbufferA->GetScissor());

    // Render
    
    const Mesh::SharedRenderingData *lastMesh = nullptr;
    for(auto &&[recordIndex, record] : Enumerate(meshes))
    {
        // Pipeline

        if(operations[recordIndex].pipeline)
        {
            commandBuffer.BindGraphicsPipeline(operations[recordIndex].pipeline);
            lastMesh = nullptr;
        }

        // Material

        MaterialPassInstance *matPassInst = operations[recordIndex].materialPassInstance;
        BindMaterialProperties(*matPassInst, keywords, commandBuffer, true);

        // Per-pass/object

        const Shader *shader = operations[recordIndex].shader;
        if(int index = shader->GetBuiltinBindingGroupIndex(ShaderInfo::BuiltinBindingGroup::Pass); index >= 0)
        {
            commandBuffer.BindGraphicsGroup(index, perPassBindingGroup);
        }
        if(int index = shader->GetBuiltinBindingGroupIndex(ShaderInfo::BuiltinBindingGroup::Object); index >= 0)
        {
            commandBuffer.BindGraphicsGroup(index, record->perObjectBindingGroup);
        }

        // Mesh

        const Mesh::SharedRenderingData *mesh = record->cachedMesh->mesh;
        if(mesh != lastMesh)
        {
            BindMesh(commandBuffer, *mesh);
            lastMesh = mesh;
        }

        // Draw

        if(mesh->GetIndexCount() > 0)
        {
            commandBuffer.DrawIndexed(mesh->GetIndexCount(), 1, 0, 0, 0);
        }
        else
        {
            commandBuffer.Draw(mesh->GetVertexCount(), 1, 0, 0);
        }
    }
}

RTRC_RENDERER_END
