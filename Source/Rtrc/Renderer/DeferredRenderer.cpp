//#include <Rtrc/Graphics/Material/BindingGroupContext.h>
//#include <Rtrc/Renderer/DeferredRenderer.h>
//#include <Rtrc/Utility/Enumerate.h>
//
//RTRC_BEGIN
//
//namespace
//{
//
//    rtrc_group(PerPassGroup_RenderGBuffers)
//    {
//        rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera);
//    };
//
//} // namespace anonymous
//
//DeferredRenderer::DeferredRenderer(Device &device, MaterialManager &materialManager)
//    : device_(device),
//      materialManager_(materialManager),
//      staticMeshPerObjectBindingGroupPool_(
//          ConstantBufferSize<StaticMeshCBuffer>, "PerObject", RHI::ShaderStageFlags::All, device)
//{
//
//}
//
//void DeferredRenderer::Render(
//    RG::RenderGraph     *renderGraph,
//    RG::TextureResource *renderTarget,
//    const Scene         &scene,
//    const RenderCamera  &camera)
//{
//    assert(!renderGraph_ && renderGraph);
//    assert(!renderTarget_ && renderTarget);
//    renderGraph_ = renderGraph;
//    renderTarget_ = renderTarget;
//    scene_ = &scene;
//    camera_ = &camera;
//
//    const uint32_t rtWidth = renderTarget->GetWidth();
//    const uint32_t rtHeight = renderTarget->GetHeight();
//
//    normal_ = renderGraph_->CreateTexture2D(RHI::TextureDesc
//    {
//        .dim                  = RHI::TextureDimension::Tex2D,
//        .format               = RHI::Format::R10G10B10A2_UNorm,
//        .width                = rtWidth,
//        .height               = rtHeight,
//        .arraySize            = 1,
//        .mipLevels            = 1,
//        .sampleCount          = 1,
//        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
//        .initialLayout        = RHI::TextureLayout::Undefined,
//        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
//    });
//    albedoMetallic_ = renderGraph_->CreateTexture2D(RHI::TextureDesc
//    {
//        .dim                  = RHI::TextureDimension::Tex2D,
//        .format               = RHI::Format::B8G8R8A8_UNorm,
//        .width                = rtWidth,
//        .height               = rtHeight,
//        .arraySize            = 1,
//        .mipLevels            = 1,
//        .sampleCount          = 1,
//        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
//        .initialLayout        = RHI::TextureLayout::Undefined,
//        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
//    });
//    roughness_ = renderGraph_->CreateTexture2D(RHI::TextureDesc
//    {
//        .dim                  = RHI::TextureDimension::Tex2D,
//        .format               = RHI::Format::B8G8R8A8_UNorm,
//        .width                = rtWidth,
//        .height               = rtHeight,
//        .arraySize            = 1,
//        .mipLevels            = 1,
//        .sampleCount          = 1,
//        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
//        .initialLayout        = RHI::TextureLayout::Undefined,
//        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
//    });
//    depth_ = renderGraph_->CreateTexture2D(RHI::TextureDesc
//    {
//        .dim                  = RHI::TextureDimension::Tex2D,
//        .format               = RHI::Format::D32,
//        .width                = rtWidth,
//        .height               = rtHeight,
//        .arraySize            = 1,
//        .mipLevels            = 1,
//        .sampleCount          = 1,
//        .usage                = RHI::TextureUsage::DepthStencil | RHI::TextureUsage::ShaderResource,
//        .initialLayout        = RHI::TextureLayout::Undefined,
//        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
//    });
//
//    auto gbufferPass = renderGraph_->CreatePass("Render GBuffers");
//    gbufferPass->Use(normal_, RG::RENDER_TARGET_WRITE);
//    gbufferPass->Use(albedoMetallic_, RG::RENDER_TARGET_WRITE);
//    gbufferPass->Use(roughness_, RG::RENDER_TARGET_WRITE);
//    gbufferPass->Use(depth_, RG::DEPTH_STENCIL);
//    gbufferPass->SetCallback([this](RG::PassContext &passContext)
//    {
//        DoRenderGBuffersPass(passContext);
//    });
//
//    auto lightingPass = renderGraph_->CreatePass("Deferred Lighting");
//    lightingPass->Use(normal_, RG::PIXEL_SHADER_TEXTURE_READ);
//    lightingPass->Use(albedoMetallic_, RG::PIXEL_SHADER_TEXTURE_READ);
//    lightingPass->Use(roughness_, RG::PIXEL_SHADER_TEXTURE_READ);
//    lightingPass->Use(depth_, RG::PIXEL_SHADER_TEXTURE_READ);
//    lightingPass->Use(renderTarget_, RG::RENDER_TARGET_WRITE);
//    lightingPass->SetCallback([this](RG::PassContext &passContext)
//    {
//        DoDeferredLightingPass(passContext);
//    });
//
//    Connect(gbufferPass, lightingPass);
//}
//
//void DeferredRenderer::DoRenderGBuffersPass(RG::PassContext &passContext)
//{
//    auto rtNormal = normal_->Get();
//    auto rtAlbedoMetallic = albedoMetallic_->Get();
//    auto rtRoughness = roughness_->Get();
//    auto rtDepth = depth_->Get();
//
//    // Render pass
//
//    CommandBuffer &cmd = passContext.GetCommandBuffer();
//    cmd.BeginRenderPass(
//    {
//        ColorAttachment
//        {
//            .renderTarget = rtNormal,
//            .loadOp = AttachmentLoadOp::Clear,
//            .storeOp = AttachmentStoreOp::Store,
//            .clearValue = { 0, 0, 0, 0 }
//        },
//        ColorAttachment
//        {
//            .renderTarget = rtAlbedoMetallic,
//            .loadOp = AttachmentLoadOp::Clear,
//            .storeOp = AttachmentStoreOp::Store,
//            .clearValue = { 0, 0, 0, 0 }
//        },
//        ColorAttachment
//        {
//            .renderTarget = rtRoughness,
//            .loadOp = AttachmentLoadOp::Clear,
//            .storeOp = AttachmentStoreOp::Store,
//            .clearValue = { 0, 0, 0, 0 }
//        }
//    },
//    DepthStencilAttachment
//    {
//        .depthStencil = rtDepth,
//        .loadOp = AttachmentLoadOp::Clear,
//        .storeOp = AttachmentStoreOp::Store,
//        .clearValue = DepthStencilClearValue{ 1, 0 }
//    });
//    RTRC_SCOPE_EXIT{ cmd.EndRenderPass(); };
//
//    KeywordValueContext keywords;
//
//    // Per pass binding groups
//
//    RC<BindingGroup> perPassGroup;
//    {
//        PerPassGroup_RenderGBuffers perPassGroupData;
//        perPassGroupData.Camera = camera_->GetConstantBufferData();
//        perPassGroup = device_.CreateBindingGroup(perPassGroupData);
//    }
//
//    BindingGroupContext bindingGroupContext;
//    bindingGroupContext.Set(PerPassGroup_RenderGBuffers::GroupName, perPassGroup);
//
//    // Upload constant buffers for static meshes
//    
//    std::vector<RC<BindingGroup>> staticMeshBindingGroups;
//    staticMeshBindingGroups.reserve(scene_->GetAllStaticMeshes().size());
//    for(auto &staticMesh : scene_->GetAllStaticMeshes())
//    {
//        StaticMeshCBuffer cbuffer;
//        cbuffer.localToWorld = staticMesh->GetLocalToWorld();
//        cbuffer.worldToLocal = staticMesh->GetWorldToLocal();
//        cbuffer.localToCamera = cbuffer.localToWorld * camera_->GetWorldToCamera();
//        cbuffer.localToClip = cbuffer.localToWorld * camera_->GetWorldToClip();
//        staticMeshBindingGroups.push_back(staticMeshPerObjectBindingGroupPool_.GetRecord(cbuffer).bindingGroup);
//    }
//    staticMeshPerObjectBindingGroupPool_.Flush();
//
//    // Render static meshes
//
//    for(auto &&[staticMeshIndex, staticMesh] : Enumerate(scene_->GetAllStaticMeshes()))
//    {
//        auto &mesh = staticMesh->GetMesh();
//        auto &matInst = staticMesh->GetMaterial();
//
//        auto submatInst = matInst->GetSubMaterialInstance("GBuffers");
//        if(!submatInst)
//            continue;
//
//        auto submat = submatInst->GetSubMaterial();
//        auto shader = submatInst->GetShader(keywords);
//        auto pipeline = renderGBuffersPipelines_.GetOrCreate(
//            submat, shader, mesh->GetLayout(), [&]
//            {
//                return device_.CreateGraphicsPipeline(
//                {
//                    .shader                 = shader,
//                    .meshLayout             = mesh->GetLayout(),
//                    .primitiveTopology      = RHI::PrimitiveTopology::TriangleList,
//                    .fillMode               = RHI::FillMode::Fill,
//                    .cullMode               = RHI::CullMode::CullBack,
//                    .enableDepthTest        = true,
//                    .enableDepthWrite       = true,
//                    .depthCompareOp         = RHI::CompareOp::Less,
//                    .colorAttachmentFormats =
//                        { normal_->GetFormat(), albedoMetallic_->GetFormat(), roughness_->GetFormat() },
//                    .depthStencilFormat     = depth_->GetFormat(),
//                });
//            });
//
//        bindingGroupContext.Set("PerObject", staticMeshBindingGroups[staticMeshIndex]);
//
//        cmd.BindPipeline(pipeline);
//        cmd.BindGraphicsGroupsForShader(bindingGroupContext, shader);
//        cmd.BindGraphicsSubMaterial(submatInst, keywords);
//        cmd.SetViewports(rtNormal->GetViewport());
//        cmd.SetScissors(rtNormal->GetScissor());
//        cmd.BindMesh(*mesh);
//
//        if(mesh->HasIndexBuffer())
//        {
//            cmd.DrawIndexed(mesh->GetIndexCount(), 1, 0, 0, 0);
//        }
//        else
//        {
//            cmd.Draw(mesh->GetVertexCount(), 1, 0, 0);
//        }
//    }
//}
//
//void DeferredRenderer::DoDeferredLightingPass(RG::PassContext &passContext)
//{
//    if(!deferredLightingPipeline_)
//    {
//        auto shader = materialManager_.GetShader("Builtin/DeferredLighting");
//        deferredLightingPipeline_ = device_.CreateGraphicsPipeline(GraphicsPipeline::Desc
//        {
//            .shader = shader,
//
//        });
//    }
//
//}
//
//RTRC_END
