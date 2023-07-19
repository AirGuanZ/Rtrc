#include <Rtrc/Renderer/GBufferBinding.h>
#include <Rtrc/Renderer/PathTracer/PathTracer.h>
#include <Rtrc/Renderer/Scene/RenderCamera.h>
#include <Rtrc/Renderer/Utility/PcgStateTexture.h>

RTRC_RENDERER_BEGIN

void PathTracer::Render(
    RG::RenderGraph &renderGraph,
    RenderCamera    &camera,
    const GBuffers  &gbuffers,
    const Vector2u  &framebufferSize,
    bool             clearBeforeRender) const
{
    RTRC_RG_SCOPED_PASS_GROUP(renderGraph, "PathTracing");

    PerCameraData &perCameraData = camera.GetPathTracingData();
    auto rngState = Prepare2DPcgStateTexture(
        renderGraph, resources_, perCameraData.rngState, framebufferSize);
    assert(!perCameraData.indirectDiffuse);
    perCameraData.indirectDiffuse = renderGraph.CreateTexture(RHI::TextureDesc
    {
        .dim    = RHI::TextureDimension::Tex2D,
        .format = RHI::Format::R32G32B32A32_Float,
        .width  = framebufferSize.x,
        .height = framebufferSize.y,
        .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess
    });

    if(clearBeforeRender)
    {
        renderGraph.CreateClearRWTexture2DPass(
            "Clear IndirectDiffuse Output", perCameraData.indirectDiffuse, Vector4f(0, 0, 0, 0));
    }
    
    rtrc_group(TracePassGroup, CS)
    {
        rtrc_inline(GBufferBindings_NormalDepth, gbuffers);

        rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera);

        rtrc_define(Texture2D, SkyLut);

        rtrc_define(RaytracingAccelerationStructure, Tlas);
        rtrc_define(StructuredBuffer,                Instances);

        rtrc_define(RWTexture2D, RngState);

        rtrc_define(RWTexture2D, Output);
        rtrc_uniform(uint2,      outputResolution);

        rtrc_uniform(uint, maxDepth);
    };

    auto &scene = camera.GetScene();

    auto pass = renderGraph.CreatePass("PathTracing");
    DeclareGBufferUses<TracePassGroup>(pass, gbuffers, RHI::PipelineStage::ComputeShader);
    pass->Use(camera.GetAtmosphereData().S, RG::CS_Texture);
    pass->Read(scene.GetRGTlas(), RHI::PipelineStage::ComputeShader);
    pass->Use(scene.GetRGInstanceBuffer(), RG::CS_StructuredBuffer);
    pass->Use(rngState, RG::CS_RWTexture);
    pass->Use(perCameraData.indirectDiffuse, RG::CS_RWTexture_WriteOnly);
    pass->SetCallback([gbuffers, perCameraData, rngState, &scene, &camera, framebufferSize, this]
    {
        auto material = resources_->GetBuiltinMaterial(BuiltinMaterial::PathTracing).get();
        auto shader = material->GetPassByIndex(PassIndex_Trace)->GetShader().get();

        TracePassGroup passData;
        FillBindingGroupGBuffers(passData, gbuffers);
        passData.Camera           = camera.GetCameraCBuffer();
        passData.SkyLut           = camera.GetAtmosphereData().S;
        passData.Tlas             = scene.GetRGTlas();
        passData.Instances        = scene.GetRGInstanceBuffer()->GetStructuredSrv(sizeof(RenderScene::TlasInstance));
        passData.RngState         = rngState;
        passData.Output           = perCameraData.indirectDiffuse;
        passData.outputResolution = framebufferSize;
        passData.maxDepth         = 5;
        auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);
        auto geometryBufferGroup = scene.GetRenderMeshes().GetGlobalGeometryBuffersBindingGroup();
        auto textureGroup = scene.GetGlobalTextureBindingGroup();
        
        auto &commandBuffer = RG::GetCurrentCommandBuffer();
        commandBuffer.BindComputePipeline(shader->GetComputePipeline());
        commandBuffer.BindComputeGroups({ passGroup, geometryBufferGroup, textureGroup });
        commandBuffer.DispatchWithThreadCount(framebufferSize);
    });
}

void PathTracer::ClearFrameData(PerCameraData &data) const
{
    data.indirectDiffuse = nullptr;
}

RTRC_RENDERER_END
