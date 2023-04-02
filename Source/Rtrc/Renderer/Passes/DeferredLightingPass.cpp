#include <Rtrc/Renderer/Common.h>
#include <Rtrc/Renderer/Passes/DeferredLightingPass.h>
#include <Rtrc/Renderer/Utility/FullscreenPrimitive.h>

RTRC_RENDERER_BEGIN

namespace DeferredLightingPassDetail
{

    rtrc_group(BindingGroup_DeferredLightingPass)
    {
        rtrc_define(Texture2D, gbufferA);
        rtrc_define(Texture2D, gbufferB);
        rtrc_define(Texture2D, gbufferC);
        rtrc_define(Texture2D, gbufferDepth);

        rtrc_define(ConstantBuffer<CameraConstantBuffer>, camera);

        rtrc_define(StructuredBuffer, pointLightBuffer);
        rtrc_uniform(uint,            pointLightCount);

        rtrc_define(StructuredBuffer, directionalLightBuffer);
        rtrc_uniform(uint,            directionalLightCount);
    };

} // namespace DeferredLightingPassDetail

DeferredLightingPass::DeferredLightingPass(ObserverPtr<Device> device, ObserverPtr<BuiltinResourceManager> builtinResources)
    : device_(device)
    , builtinResources_(builtinResources)
    , pipelineCache_(device)
    , lightBufferPool_(device, RHI::BufferUsage::ShaderStructuredBuffer)
{
    perPassBindingGroupLayout_ =
        device_->CreateBindingGroupLayout<DeferredLightingPassDetail::BindingGroup_DeferredLightingPass>();

    shaderTemplate_ = builtinResources_->GetBuiltinMaterial(BuiltinMaterial::DeferredLighting2)
                                       ->GetPassByIndex(0)->GetShaderTemplate();

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
    const RenderGraphInput     &rgInput)
{
    RenderGraphOutput ret;

    ret.lightingPass = renderGraph.CreatePass("Deferred Lighting");

    ret.lightingPass->Use(rgInput.inGBuffers.a, RG::PIXEL_SHADER_TEXTURE);
    ret.lightingPass->Use(rgInput.inGBuffers.b, RG::PIXEL_SHADER_TEXTURE);
    ret.lightingPass->Use(rgInput.inGBuffers.c, RG::PIXEL_SHADER_TEXTURE);
    ret.lightingPass->Use(rgInput.inGBuffers.depth, RG::UseInfo
    {
        .layout   = RHI::TextureLayout::DepthShaderTexture_StencilReadOnlyAttachment,
        .stages   = RHI::PipelineStage::FragmentShader | RHI::PipelineStage::DepthStencil,
        .accesses = RHI::ResourceAccess::TextureRead | RHI::ResourceAccess::DepthStencilRead
    });
    ret.lightingPass->Use(rgInput.outRenderTarget, RG::COLOR_ATTACHMENT_WRITEONLY);

    ret.lightingPass->SetCallback([this, rgInput, &scene](RG::PassContext &context)
    {
        DoDeferredLighting(scene, rgInput, context);
    });

    return ret;
}

void DeferredLightingPass::DoDeferredLighting(
    const CachedScenePerCamera &scene,
    const RenderGraphInput     &rgInput,
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
            data.range = light->GetRange();
        }
        else if(light->GetType() == Light::Type::Directional)
        {
            auto &data = directionalLightData.emplace_back();
            data.direction = light->GetDirection();
            data.color = light->GetIntensity() * light->GetColor();
        }
        else
        {
            LogWarningUnformatted(
                "DeferredLightPass: unknown light type. Only point/directional lights are supported");
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
    
    // Render Pass

    RC<Texture> renderTarget = rgInput.outRenderTarget->Get(context);

    CommandBuffer &commandBuffer = context.GetCommandBuffer();
    commandBuffer.BeginRenderPass(ColorAttachment
    {
        .renderTargetView = renderTarget->CreateRtv(),
        .loadOp           = AttachmentLoadOp::DontCare,
        .storeOp          = AttachmentStoreOp::Store
    });
    RTRC_SCOPE_EXIT{ commandBuffer.EndRenderPass(); };

    // Pipeline

    KeywordContext keywords;

    GraphicsPipeline::Desc pipelineDesc = pipelineTemplate_;
    RC<Shader> shader = shaderTemplate_->GetShader(keywords);
    pipelineDesc.shader = shader;
    pipelineDesc.colorAttachmentFormats = { rgInput.outRenderTarget->GetFormat() };
    pipelineDesc.depthStencilFormat = rgInput.inGBuffers.depth->GetFormat();

    RC<GraphicsPipeline> pipeline = pipelineCache_.GetGraphicsPipeline(pipelineDesc);
    commandBuffer.BindGraphicsPipeline(pipeline);

    pipelineCache_.CommitChanges();

    // Dynamic states

    commandBuffer.SetStencilReferenceValue(std::to_underlying(StencilBit::None));
    commandBuffer.SetViewports(renderTarget->GetViewport());
    commandBuffer.SetScissors(renderTarget->GetScissor());

    // Per-pass binding group

    BindingGroup_DeferredLightingPass passData;
    passData.gbufferA               = rgInput.inGBuffers.a->Get(context);
    passData.gbufferB               = rgInput.inGBuffers.b->Get(context);
    passData.gbufferC               = rgInput.inGBuffers.c->Get(context);
    passData.gbufferDepth           = rgInput.inGBuffers.depth->Get(context);
    passData.camera                 = scene.GetCameraCBuffer();
    passData.pointLightBuffer       = pointLightBuffer->GetStructuredSrv(sizeof(PointLightShadingData));
    passData.pointLightCount        = static_cast<uint32_t>(pointLightData.size());
    passData.directionalLightBuffer = directionalLightBuffer->GetStructuredSrv(sizeof(DirectionalLightShadingData));
    passData.directionalLightCount  = static_cast<uint32_t>(directionalLightData.size());
    auto passBindingGroup = device_->CreateBindingGroup(passData, perPassBindingGroupLayout_);

    commandBuffer.BindGraphicsGroup(
        shader->GetBuiltinBindingGroupIndex(ShaderBindingLayoutInfo::BuiltinBindingGroup::Pass), passBindingGroup);

    // Mesh

    const Mesh mesh = GetFullscreenTriangle(*device_, scene.GetCamera());
    BindMesh(commandBuffer, mesh);

    // Render

    commandBuffer.Draw(mesh.GetVertexCount(), 1, 0, 0);
}

RTRC_RENDERER_END
