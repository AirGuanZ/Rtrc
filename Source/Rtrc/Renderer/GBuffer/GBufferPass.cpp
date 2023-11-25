#include <ranges>

#include <Rtrc/Renderer/Common.h>
#include <Rtrc/Renderer/GBuffer/GBufferPass.h>
#include <Rtrc/Renderer/GPUScene/RenderCamera.h>
#include <Core/Enumerate.h>

RTRC_RENDERER_BEGIN

namespace GBufferPassDetail
{

    rtrc_group(BindingGroup_GBufferPass)
    {
        rtrc_define(ConstantBuffer<CameraData>, Camera);
        rtrc_define(StructuredBuffer,           PerObjectDataBuffer);
    };

} // namespace GBufferPassDetail

GBufferPass::GBufferPass(ObserverPtr<Device> device)
    : device_(device), gbufferPipelineCache_(device)
{

}

void GBufferPass::Render(
    const RenderCamera      &sceneCamera,
    const RC<BindingGroup>  &bindlessTextureGroup,
    RG::RenderGraph         &renderGraph)
{
    auto &gbuffers = sceneCamera.GetGBuffers();
    auto gbufferPass = renderGraph.CreatePass("Render GBuffers");
    gbufferPass->Use(gbuffers.currNormal, RG::ColorAttachmentWriteOnly);
    gbufferPass->Use(gbuffers.albedoMetallic, RG::ColorAttachmentWriteOnly);
    gbufferPass->Use(gbuffers.roughness, RG::ColorAttachmentWriteOnly);
    gbufferPass->Use(gbuffers.depthStencil, RG::DepthStencilAttachment);
    gbufferPass->SetCallback([this, gbuffers, &sceneCamera, bindlessTextureGroup]
    {
        DoRenderGBuffers(sceneCamera, bindlessTextureGroup, gbuffers);
    });
}

std::vector<GBufferPass::MaterialGroup> GBufferPass::CollectPipelineGroups(const RenderCamera &scene) const
{
    std::vector<MaterialGroup> materialGroups;

    const LegacyMaterial *lastMaterial = nullptr;
    const MaterialRenderingCache *lastMaterialRenderingData = nullptr;
    const MeshRenderingCache *lastMeshRenderingData = nullptr;

    std::vector<RenderCamera::MeshRendererRecord *> rendererRecords =
        { scene.GetMeshRendererRecords().begin(), scene.GetMeshRendererRecords().end() };
    std::ranges::sort(
        rendererRecords,
        [](const RenderCamera::MeshRendererRecord *lhs, const RenderCamera::MeshRendererRecord *rhs)
        {
            return std::make_tuple(
                        lhs->materialCache->material,
                        lhs->materialCache->materialInstance,
                        lhs->meshCache->mesh->GetLayout(),
                        lhs->meshCache->mesh) <
                    std::make_tuple(
                        rhs->materialCache->material,
                        rhs->materialCache->materialInstance,
                        rhs->meshCache->mesh->GetLayout(),
                        rhs->meshCache->mesh);
    });

    for(auto &&[recordIndex, record] : Enumerate(scene.GetMeshRendererRecords()))
    {
        const int gbufferPassIndex = record->materialCache->gbufferPassIndex;
        if(gbufferPassIndex < 0)
            continue;

        if(record->materialCache->materialInstance->GetMaterial().get() != lastMaterial)
        {
            auto &group = materialGroups.emplace_back();
            group.material          = record->materialCache->materialInstance->GetMaterial().get();
            group.shaderTemplate    = group.material->GetPasses()[gbufferPassIndex]->GetShaderTemplate().get();
            group.supportInstancing = group.shaderTemplate->HasBuiltinKeyword(BuiltinKeyword::EnableInstance);
            group.gbufferPassIndex  = gbufferPassIndex;
            lastMaterial = group.material;
            lastMaterialRenderingData = nullptr;
        }

        MaterialGroup &materialGroup = materialGroups.back();
        assert(materialGroup.material == record->materialCache->materialInstance->GetMaterial().get());
        if(record->materialCache != lastMaterialRenderingData)
        {
            auto &group = materialGroup.materialInstanceGroups.emplace_back();
            group.materialCache = record->materialCache;
            lastMaterialRenderingData = group.materialCache;
            lastMeshRenderingData = nullptr;
        }

        MaterialInstanceGroup &materialInstanceGroup = materialGroup.materialInstanceGroups.back();
        assert(materialInstanceGroup.materialCache == record->materialCache);

        if(record->meshCache != lastMeshRenderingData)
        {
            auto &group = materialInstanceGroup.meshGroups.emplace_back();
            group.meshCache = record->meshCache;
            group.objectDataOffset  = static_cast<uint32_t>(recordIndex);
            group.objectCount       = 0;
            lastMeshRenderingData = group.meshCache;
        }

        MeshGroup &meshGroup = materialInstanceGroup.meshGroups.back();
        assert(meshGroup.meshCache == record->meshCache);
        assert(meshGroup.objectDataOffset + meshGroup.objectCount == recordIndex);
        ++meshGroup.objectCount;
    }

    return materialGroups;
}

void GBufferPass::DoRenderGBuffers(
    const RenderCamera     &camera,
    const RC<BindingGroup> &bindlessTextureGroup,
    const GBuffers         &gbuffers)
{
    auto gbufferA = gbuffers.currNormal;
    auto gbufferB = gbuffers.albedoMetallic;
    auto gbufferC = gbuffers.roughness;
    auto gbufferDepth = gbuffers.depthStencil;

    // Render Pass

    CommandBuffer &commandBuffer = RG::GetCurrentCommandBuffer();
    commandBuffer.BeginRenderPass(
        {
            ColorAttachment
            {
                .renderTargetView = gbufferA->GetRtvImm(),
                .loadOp           = AttachmentLoadOp::Clear,
                .storeOp          = AttachmentStoreOp::Store,
                .clearValue       = { 0, 0, 0, 0 }
            },
            ColorAttachment
            {
                .renderTargetView = gbufferB->GetRtvImm(),
                .loadOp           = AttachmentLoadOp::Clear,
                .storeOp          = AttachmentStoreOp::Store,
                .clearValue       = { 0, 0, 0, 0 }
            },
            ColorAttachment
            {
                .renderTargetView = gbufferC->GetRtvImm(),
                .loadOp           = AttachmentLoadOp::Clear,
                .storeOp          = AttachmentStoreOp::Store,
                .clearValue       = { 0, 0, 0, 0 }
            }
        },
        DepthStencilAttachment
        {
            .depthStencilView = gbufferDepth->GetDsvImm(),
            .loadOp           = AttachmentLoadOp::Clear,
            .storeOp          = AttachmentStoreOp::Store,
            .clearValue       = { 1, 0 }
        });
    RTRC_SCOPE_EXIT{ commandBuffer.EndRenderPass(); };

    // Dynamic pipeline states

    commandBuffer.SetStencilReferenceValue(std::to_underlying(StencilBit::Regular));
    commandBuffer.SetViewports(gbufferA->GetViewport());
    commandBuffer.SetScissors(gbufferA->GetScissor());

    // Per-pass binding group

    RC<BindingGroup> perPassBindingGroup;
    {
        GBufferPassDetail::BindingGroup_GBufferPass passData;
        passData.Camera = camera.GetCameraCBuffer();
        passData.PerObjectDataBuffer = camera.GetMeshRendererPerObjectDataBuffer();

        RTRC_STATIC_BINDING_GROUP_LAYOUT(device_, GBufferPassDetail::BindingGroup_GBufferPass, bindingGroupLayout);
        perPassBindingGroup = device_->CreateBindingGroup(passData, bindingGroupLayout);
    }
    
    // Render

    FastKeywordContext keywords;

    const std::vector<MaterialGroup> materialGroups = CollectPipelineGroups(camera);

    RC<GraphicsPipeline> lastPipeline;
    for(const MaterialGroup &materialGroup : materialGroups)
    {
        RC<GraphicsPipeline> pipelineInstanceOn, pipelineInstanceOff;
        const MeshLayout *lastMeshLayout = nullptr;

        for(const MaterialInstanceGroup &materialInstanceGroup : materialGroup.materialInstanceGroups)
        {
            auto materialInstance = materialInstanceGroup.materialCache->materialInstance;
            auto materialPassInstance = materialInstance->GetPassInstance(materialGroup.gbufferPassIndex);
            bool shouldBindMaterialProperties = true;

            for(const MeshGroup &meshGroup : materialInstanceGroup.meshGroups)
            {
                const Mesh *mesh = meshGroup.meshCache->mesh;
                mesh->Bind(commandBuffer);
                
                const MeshLayout *meshLayout = mesh->GetLayout();
                if(meshLayout != lastMeshLayout)
                {
                    pipelineInstanceOn = {};
                    pipelineInstanceOff = {};
                    lastMeshLayout = meshLayout;
                }

                const bool enableInstancing = materialGroup.supportInstancing && meshGroup.objectCount > 0;
                keywords.Set(GetBuiltinShaderKeyword(BuiltinKeyword::EnableInstance), enableInstancing ? 1 : 0);
                RC<Shader> shader = materialGroup.shaderTemplate->GetVariant(keywords, true);
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
                        .depthStencilFormat = gbuffers.depthStencil->GetFormat()
                    });
                }

                if(pipeline != lastPipeline)
                {
                    using enum Shader::BuiltinBindingGroup;

                    lastPipeline = pipeline;
                    shouldBindMaterialProperties = true;
                    commandBuffer.BindGraphicsPipeline(pipeline);

                    const int passGroupIndex = shader->GetBuiltinBindingGroupIndex(Pass);
                    if(passGroupIndex >= 0)
                    {
                        commandBuffer.BindGraphicsGroup(passGroupIndex, perPassBindingGroup);
                    }

                    const int texGroupIndex = shader->GetBuiltinBindingGroupIndex(BindlessTexture);
                    if(texGroupIndex >= 0)
                    {
                        commandBuffer.BindGraphicsGroup(texGroupIndex, bindlessTextureGroup);
                    }
                }

                if(shouldBindMaterialProperties)
                {
                    BindMaterialProperties(*materialPassInstance, keywords, commandBuffer, true);
                }

                if(enableInstancing)
                {
                    const uint32_t perObjectDataOffset = meshGroup.objectDataOffset;
                    commandBuffer.SetGraphicsPushConstants(0, perObjectDataOffset);
                    if(meshGroup.meshCache->mesh->GetIndexCount() > 0)
                    {
                        commandBuffer.DrawIndexed(mesh->GetIndexCount(), meshGroup.objectCount, 0, 0, 0);
                    }
                    else
                    {
                        commandBuffer.Draw(mesh->GetVertexCount(), meshGroup.objectCount, 0, 0);
                    }
                }
                else
                {
                    if(mesh->GetIndexCount() > 0)
                    {
                        for(uint32_t i = 0; i < meshGroup.objectCount; ++i)
                        {
                            const uint32_t perObjectDataOffset = meshGroup.objectDataOffset + i;
                            commandBuffer.SetGraphicsPushConstants(0, perObjectDataOffset);
                            commandBuffer.DrawIndexed(mesh->GetIndexCount(), 1, 0, 0, 0);
                        }
                    }
                    else
                    {
                        for(uint32_t i = 0; i < meshGroup.objectCount; ++i)
                        {
                            const uint32_t perObjectDataOffset = meshGroup.objectDataOffset + i;
                            commandBuffer.SetGraphicsPushConstants(0, perObjectDataOffset);
                            commandBuffer.Draw(mesh->GetVertexCount(), 1, 0, 0);
                        }
                    }
                }
            }
        }
    }
}

RTRC_RENDERER_END
