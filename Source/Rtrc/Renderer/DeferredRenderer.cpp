#include <Rtrc/Graphics/Material/BindingGroupContext.h>
#include <Rtrc/Renderer/DeferredRenderer.h>
#include <Rtrc/Renderer/Utility/FullscreenPrimitive.h>
#include <Rtrc/Renderer/Utility/ScopedGPUDebugEvent.h>
#include <Rtrc/Utility/Enumerate.h>

RTRC_BEGIN

namespace
{

    rtrc_group(PerPassGroup_RenderGBuffers)
    {
        rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera);
    };

    rtrc_struct(PerPassConstantBuffer_DeferredLighting)
    {
        rtrc_var(Vector4f,                       GBufferSize);
        rtrc_var(CameraConstantBuffer,           camera);
        rtrc_var(DirectionalLightConstantBuffer, directionalLight);
    };

    rtrc_group(PerPassGroup_DeferredLighting)
    {
        rtrc_define(ConstantBuffer<PerPassConstantBuffer_DeferredLighting>, CBuffer_DeferredLighting);
        rtrc_define(Texture2D<float4>, GBufferA);
        rtrc_define(Texture2D<float4>, GBufferB);
        rtrc_define(Texture2D<float4>, GBufferC);
        rtrc_define(Texture2D<float>,  GBufferDepth);
    };

} // namespace anonymous

DeferredRenderer::DeferredRenderer(Device &device, MaterialManager &materialManager)
    : device_(device)
    , materialManager_(materialManager)
    , staticMeshConstantBufferBatch_(
          ConstantBufferSize<StaticMeshCBuffer>, "PerObject", RHI::ShaderStageFlags::All, device)
{

}

void DeferredRenderer::AddToRenderGraph(
    RG::RenderGraph     *renderGraph,
    RG::TextureResource *renderTarget,
    const Scene         &scene,
    const Camera        &camera)
{
    assert(!renderGraph_ && renderGraph);
    assert(!renderTarget_ && renderTarget);
    renderGraph_ = renderGraph;
    renderTarget_ = renderTarget;
    scene_ = &scene;
    camera_ = &camera;

    const uint32_t rtWidth = renderTarget->GetWidth();
    const uint32_t rtHeight = renderTarget->GetHeight();

    fullscreenQuadWithRays_ = GetFullscreenQuad(device_, camera);
    fullscreenTriangleWithRays_ = GetFullscreenTriangle(device_, camera);

    auto normal = renderGraph_->CreateTexture2D(RHI::TextureDesc
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
    });
    auto albedoMetallic = renderGraph_->CreateTexture2D(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::B8G8R8A8_UNorm,
        .width                = rtWidth,
        .height               = rtHeight,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });
    auto roughness = renderGraph_->CreateTexture2D(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::B8G8R8A8_UNorm,
        .width                = rtWidth,
        .height               = rtHeight,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });
    auto depth = renderGraph_->CreateTexture2D(RHI::TextureDesc
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
    });
    auto image = renderGraph_->CreateTexture2D(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R11G11B10_UFloat,
        .width                = rtWidth,
        .height               = rtHeight,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });

    RG::Pass *gbufferPass = renderGraph_->CreatePass("Render GBuffers");
    {
        gbufferPass->Use(normal, RG::RENDER_TARGET_WRITE);
        gbufferPass->Use(albedoMetallic, RG::RENDER_TARGET_WRITE);
        gbufferPass->Use(roughness, RG::RENDER_TARGET_WRITE);
        gbufferPass->Use(depth, RG::DEPTH_STENCIL);

        RenderGBuffersPassData passData;
        passData.normal         = normal;
        passData.albedoMetallic = albedoMetallic;
        passData.roughness      = roughness;
        passData.depth          = depth;

        gbufferPass->SetCallback([this, passData](RG::PassContext &passContext)
        {
            DoRenderGBuffersPass(passContext, passData);
        });
    }

    RG::Pass *lightingPass = renderGraph_->CreatePass("Deferred Lighting");
    {
        lightingPass->Use(normal, RG::PIXEL_SHADER_TEXTURE_READ);
        lightingPass->Use(albedoMetallic, RG::PIXEL_SHADER_TEXTURE_READ);
        lightingPass->Use(roughness, RG::PIXEL_SHADER_TEXTURE_READ);
        lightingPass->Use(depth, RG::PIXEL_SHADER_TEXTURE_READ);
        lightingPass->Use(image, RG::RENDER_TARGET_WRITE);

        DeferredLightingPassData passData;
        passData.normal         = normal;
        passData.albedoMetallic = albedoMetallic;
        passData.roughness      = roughness;
        passData.depth          = depth;
        passData.image          = image;

        lightingPass->SetCallback([this, passData](RG::PassContext &passContext)
        {
            DoDeferredLightingPass(passContext, passData);
        });
    }

    Connect(gbufferPass, lightingPass);
}

void DeferredRenderer::DoRenderGBuffersPass(RG::PassContext &passContext, const RenderGBuffersPassData &passData)
{
    CommandBuffer &cmd = passContext.GetCommandBuffer();
    RTRC_SCOPED_GPU_EVENT(cmd, "Render GBuffers", {});

    auto rtNormal = passData.normal->Get();
    auto rtAlbedoMetallic = passData.albedoMetallic->Get();
    auto rtRoughness = passData.roughness->Get();
    auto rtDepth = passData.depth->Get();

    // Render pass

    cmd.BeginRenderPass(
    {
        ColorAttachment
        {
            .renderTargetView = rtNormal->CreateRTV(),
            .loadOp = AttachmentLoadOp::Clear,
            .storeOp = AttachmentStoreOp::Store,
            .clearValue = { 0, 0, 0, 0 }
        },
        ColorAttachment
        {
            .renderTargetView = rtAlbedoMetallic->CreateRTV(),
            .loadOp = AttachmentLoadOp::Clear,
            .storeOp = AttachmentStoreOp::Store,
            .clearValue = { 0, 0, 0, 0 }
        },
        ColorAttachment
        {
            .renderTargetView = rtRoughness->CreateRTV(),
            .loadOp = AttachmentLoadOp::Clear,
            .storeOp = AttachmentStoreOp::Store,
            .clearValue = { 0, 0, 0, 0 }
        }
    },
    DepthStencilAttachment
    {
        .depthStencilView = rtDepth->CreateDSV(),
        .loadOp = AttachmentLoadOp::Clear,
        .storeOp = AttachmentStoreOp::Store,
        .clearValue = DepthStencilClearValue{ 1, 0 }
    });
    RTRC_SCOPE_EXIT{ cmd.EndRenderPass(); };

    KeywordValueContext keywords;

    // Per pass binding groups

    RC<BindingGroup> perPassGroup;
    {
        PerPassGroup_RenderGBuffers perPassGroupData;
        perPassGroupData.Camera = camera_->GetConstantBufferData();
        perPassGroup = device_.CreateBindingGroup(perPassGroupData);
    }

    BindingGroupContext bindingGroupContext;
    bindingGroupContext.Set(PerPassGroup_RenderGBuffers::GroupName, perPassGroup);

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

    for(auto &&[staticMeshIndex, staticMesh] : Enumerate(scene_->GetAllStaticMeshes()))
    {
        auto &mesh = staticMesh->GetMesh();
        auto &matInst = staticMesh->GetMaterial();

        auto submatInst = matInst->GetSubMaterialInstance("GBuffers");
        if(!submatInst)
            continue;

        auto submat = submatInst->GetSubMaterial();
        auto shader = submatInst->GetShader(keywords);
        auto pipeline = renderGBuffersPipelines_.GetOrCreate(
            submat, shader, mesh->GetLayout(), [&]
            {
                return device_.CreateGraphicsPipeline(
                {
                    .shader                 = shader,
                    .meshLayout             = mesh->GetLayout(),
                    .primitiveTopology      = RHI::PrimitiveTopology::TriangleList,
                    .fillMode               = RHI::FillMode::Fill,
                    .cullMode               = RHI::CullMode::CullBack,
                    .enableDepthTest        = true,
                    .enableDepthWrite       = true,
                    .depthCompareOp         = RHI::CompareOp::Less,
                    .colorAttachmentFormats =
                    {
                        passData.normal->GetFormat(),
                        passData.albedoMetallic->GetFormat(),
                        passData.roughness->GetFormat()
                    },
                    .depthStencilFormat     = passData.depth->GetFormat(),
                });
            });

        bindingGroupContext.Set("PerObject", staticMeshBindingGroups[staticMeshIndex]);

        cmd.BindPipeline(pipeline);
        bindingGroupContext.BindForGraphicsPipeline(cmd, shader);
        submatInst->BindGraphicsProperties(keywords, cmd);
        cmd.SetViewports(rtNormal->GetViewport());
        cmd.SetScissors(rtNormal->GetScissor());
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

void DeferredRenderer::DoDeferredLightingPass(RG::PassContext &passContext, const DeferredLightingPassData &passData)
{
    CommandBuffer &cmd = passContext.GetCommandBuffer();
    RTRC_SCOPED_GPU_EVENT(cmd, "Deferred Lighting", {});

    if(!deferredLightingPipeline_)
    {
        auto shader = materialManager_.GetShader("Builtin/DeferredLighting");
        deferredLightingPipeline_ = device_.CreateGraphicsPipeline(GraphicsPipeline::Desc
        {
            .shader                 = shader,
            .meshLayout             = fullscreenTriangleWithRays_.GetLayout(),
            .colorAttachmentFormats = { passData.image->GetFormat() }
        });
    }

    // TODO
}

RTRC_END
