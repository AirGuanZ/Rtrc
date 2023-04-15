#include <ranges>

#include <Rtrc/Renderer/Common.h>
#include <Rtrc/Renderer/Passes/GBufferPass.h>
#include <Rtrc/Utility/Enumerate.h>

RTRC_RENDERER_BEGIN

namespace GBufferPassDetail
{

    rtrc_group(BindingGroup_GBufferPass)
    {
        rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera);
        rtrc_define(StructuredBuffer,                     PerObjectDataBuffer);
    };

} // namespace GBufferPassDetail

GBufferPass::GBufferPass(ObserverPtr<Device> device)
    : device_(device), gbufferPipelineCache_(device)
{
    perPassBindingGroupLayout_ = device_->CreateBindingGroupLayout<GBufferPassDetail::BindingGroup_GBufferPass>();
}

GBufferPass::RenderGraphOutput GBufferPass::RenderGBuffers(
    const CachedScenePerCamera &scene,
    const RC<BindingGroup>     &bindlessTextureGroup,
    RG::RenderGraph            &renderGraph,
    const Vector2u             &rtSize)
{
    RenderGraphOutput ret;
    ret.gbuffers = AllocateGBuffers(renderGraph, rtSize);
    ret.gbufferPass = renderGraph.CreatePass("Render GBuffers");
    ret.gbufferPass->Use(ret.gbuffers.normal, RG::COLOR_ATTACHMENT_WRITEONLY);
    ret.gbufferPass->Use(ret.gbuffers.albedoMetallic, RG::COLOR_ATTACHMENT_WRITEONLY);
    ret.gbufferPass->Use(ret.gbuffers.roughness, RG::COLOR_ATTACHMENT_WRITEONLY);
    ret.gbufferPass->Use(ret.gbuffers.depth, RG::DEPTH_STENCIL_ATTACHMENT);
    ret.gbufferPass->SetCallback([this, gbuffers = ret.gbuffers, &scene, bindlessTextureGroup](RG::PassContext &context)
    {
        DoRenderGBuffers(context, scene, bindlessTextureGroup, gbuffers);
    });
    return ret;
}

GBuffers GBufferPass::AllocateGBuffers(RG::RenderGraph &renderGraph, const Vector2u &rtSize)
{
    GBuffers ret;
    ret.normal = renderGraph.CreateTexture(RHI::TextureDesc
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
    ret.albedoMetallic = renderGraph.CreateTexture(RHI::TextureDesc
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
    ret.roughness = renderGraph.CreateTexture(RHI::TextureDesc
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

std::vector<GBufferPass::MaterialGroup> GBufferPass::CollectPipelineGroups(const CachedScenePerCamera &scene) const
{
    std::vector<MaterialGroup> materialGroups;

    const Material                              *lastMaterial              = nullptr;
    const MaterialInstance::SharedRenderingData *lastMaterialRenderingData = nullptr;
    const Mesh::SharedRenderingData             *lastMeshRenderingData     = nullptr;

    for(auto &&[recordIndex, record] : Enumerate(scene.GetStaticMeshes()))
    {
        if(record->gbufferPassIndex < 0)
            continue;

        if(record->cachedMaterial->material != lastMaterial)
        {
            auto &group = materialGroups.emplace_back();
            group.material          = record->cachedMaterial->material;
            group.shaderTemplate    = group.material->GetPasses()[record->gbufferPassIndex]->GetShaderTemplate().get();
            group.supportInstancing = group.shaderTemplate->HasBuiltinKeyword(BuiltinKeyword::EnableInstance);
            group.gbufferPassIndex  = record->gbufferPassIndex;
            lastMaterial = group.material;
            lastMaterialRenderingData = nullptr;
        }

        MaterialGroup &materialGroup = materialGroups.back();
        assert(materialGroup.material == record->cachedMaterial->material);

        if(record->cachedMaterial->materialRenderingData != lastMaterialRenderingData)
        {
            auto &group = materialGroup.materialInstanceGroups.emplace_back();
            group.materialRenderingData = record->cachedMaterial->materialRenderingData;
            lastMaterialRenderingData = group.materialRenderingData;
            lastMeshRenderingData = nullptr;
        }

        MaterialInstanceGroup &materialInstanceGroup = materialGroup.materialInstanceGroups.back();
        assert(materialInstanceGroup.materialRenderingData == record->cachedMaterial->materialRenderingData);

        if(record->cachedMesh->meshRenderingData != lastMeshRenderingData)
        {
            auto &group = materialInstanceGroup.meshGroups.emplace_back();
            group.meshRenderingData = record->cachedMesh->meshRenderingData;
            group.objectDataOffset  = static_cast<uint32_t>(recordIndex);
            group.objectCount       = 0;
            lastMeshRenderingData = group.meshRenderingData;
        }

        MeshGroup &meshGroup = materialInstanceGroup.meshGroups.back();
        assert(meshGroup.meshRenderingData == record->cachedMesh->meshRenderingData);
        assert(meshGroup.objectDataOffset + meshGroup.objectCount == recordIndex);
        ++meshGroup.objectCount;
    }

    return materialGroups;
}

void GBufferPass::DoRenderGBuffers(
    RG::PassContext            &passContext,
    const CachedScenePerCamera &scene,
    const RC<BindingGroup>     &bindlessTextureGroup,
    const GBuffers             &gbuffers)
{
    auto gbufferA = gbuffers.normal->Get(passContext);
    auto gbufferB = gbuffers.albedoMetallic->Get(passContext);
    auto gbufferC = gbuffers.roughness->Get(passContext);
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

    // Sort static meshes by material

    struct MeshRecord
    {
        const CachedScenePerCamera::StaticMeshRecord *mesh;
        uint32_t perObjectDataOffset;
    };

    std::vector<MeshRecord> meshes;
    for(auto &&[index, record] : Enumerate(scene.GetStaticMeshes()))
    {
        if(record->gbufferPassIndex >= 0)
        {
            meshes.push_back({ record, static_cast<uint32_t>(index) });
        }
    }
    assert(std::ranges::is_sorted(meshes, [](const MeshRecord &lhs, const MeshRecord &rhs)
    {
        return std::make_tuple(
                    lhs.mesh->cachedMaterial->materialId,
                    lhs.mesh->cachedMaterial,
                    lhs.mesh->cachedMesh->meshRenderingData->GetLayout(),
                    lhs.mesh->cachedMesh) <
               std::make_tuple(
                    rhs.mesh->cachedMaterial->materialId,
                    rhs.mesh->cachedMaterial,
                    rhs.mesh->cachedMesh->meshRenderingData->GetLayout(),
                    rhs.mesh->cachedMesh);
    }));

    // Dynamic pipeline states

    commandBuffer.SetStencilReferenceValue(std::to_underlying(StencilBit::Regular));
    commandBuffer.SetViewports(gbufferA->GetViewport());
    commandBuffer.SetScissors(gbufferA->GetScissor());

    // Per-pass binding group

    RC<BindingGroup> perPassBindingGroup;
    {
        GBufferPassDetail::BindingGroup_GBufferPass passData;
        passData.Camera = scene.GetCameraCBuffer();
        passData.PerObjectDataBuffer = scene.GetStaticMeshPerObjectData();
        perPassBindingGroup = device_->CreateBindingGroup(passData, perPassBindingGroupLayout_);
    }
    
    // Render

    KeywordContext keywords;

    const std::vector<MaterialGroup> materialGroups = CollectPipelineGroups(scene);

    RC<GraphicsPipeline> lastPipeline;
    for(const MaterialGroup &materialGroup : materialGroups)
    {
        RC<GraphicsPipeline> pipelineInstanceOn, pipelineInstanceOff;
        const MeshLayout *lastMeshLayout = nullptr;

        for(const MaterialInstanceGroup &materialInstanceGroup : materialGroup.materialInstanceGroups)
        {
            auto materialRenderingData = materialInstanceGroup.materialRenderingData;
            auto materialPassInstance = materialRenderingData->GetPassInstance(materialGroup.gbufferPassIndex);
            bool shouldBindMaterialProperties = true;

            for(const MeshGroup &meshGroup : materialInstanceGroup.meshGroups)
            {
                BindMesh(commandBuffer, *meshGroup.meshRenderingData);

                const MeshLayout *meshLayout = meshGroup.meshRenderingData->GetLayout();
                if(meshLayout != lastMeshLayout)
                {
                    pipelineInstanceOn = {};
                    pipelineInstanceOff = {};
                    lastMeshLayout = meshLayout;
                }

                const bool enableInstancing = materialGroup.supportInstancing && meshGroup.objectCount > 0;
                keywords.Set(GetBuiltinKeyword(BuiltinKeyword::EnableInstance), enableInstancing ? 1 : 0);
                RC<Shader> shader = materialGroup.shaderTemplate->GetShader(keywords);
                RC<GraphicsPipeline> &pipeline = enableInstancing ? pipelineInstanceOn : pipelineInstanceOff;

                if(!pipeline)
                {
                    pipeline = gbufferPipelineCache_.GetGraphicsPipeline(
            {
                        .shader            = shader,
                        .meshLayout        = meshLayout,
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
                }

                if(pipeline != lastPipeline)
                {
                    lastPipeline = pipeline;
                    shouldBindMaterialProperties = true;
                    commandBuffer.BindGraphicsPipeline(pipeline);

                    const int passBindingGroupIndex = shader->GetBuiltinBindingGroupIndex(
                        Shader::BuiltinBindingGroup::Pass);
                    if(passBindingGroupIndex >= 0)
                    {
                        commandBuffer.BindGraphicsGroup(passBindingGroupIndex, perPassBindingGroup);
                    }

                    const int bindlessTextureGroupIndex = shader->GetBuiltinBindingGroupIndex(
                        Shader::BuiltinBindingGroup::BindlessTexture);
                    if(bindlessTextureGroupIndex >= 0)
                    {
                        commandBuffer.BindGraphicsGroup(bindlessTextureGroupIndex, bindlessTextureGroup);
                    }
                }

                if(shouldBindMaterialProperties)
                {
                    BindMaterialProperties(*materialPassInstance, keywords, commandBuffer, true);
                }

                if(enableInstancing)
                {
                    const uint32_t perObjectDataOffset = meshGroup.objectDataOffset;
                    commandBuffer.SetGraphicsPushConstants(perObjectDataOffset);
                    if(meshGroup.meshRenderingData->GetIndexCount() > 0)
                    {
                        commandBuffer.DrawIndexed(
                            meshGroup.meshRenderingData->GetIndexCount(), meshGroup.objectCount, 0, 0, 0);
                    }
                    else
                    {
                        commandBuffer.Draw(
                            meshGroup.meshRenderingData->GetVertexCount(), meshGroup.objectCount, 0, 0);
                    }
                }
                else
                {
                    if(meshGroup.meshRenderingData->GetIndexCount() > 0)
                    {
                        for(uint32_t i = 0; i < meshGroup.objectCount; ++i)
                        {
                            const uint32_t perObjectDataOffset = meshGroup.objectDataOffset + i;
                            commandBuffer.SetGraphicsPushConstants(perObjectDataOffset);
                            commandBuffer.DrawIndexed(
                                meshGroup.meshRenderingData->GetIndexCount(), 1, 0, 0, 0);
                        }
                    }
                    else
                    {
                        for(uint32_t i = 0; i < meshGroup.objectCount; ++i)
                        {
                            const uint32_t perObjectDataOffset = meshGroup.objectDataOffset + i;
                            commandBuffer.SetGraphicsPushConstants(perObjectDataOffset);
                            commandBuffer.Draw(
                                meshGroup.meshRenderingData->GetVertexCount(), 1, 0, 0);
                        }
                    }
                }
            }
        }
    }
}

RTRC_RENDERER_END
