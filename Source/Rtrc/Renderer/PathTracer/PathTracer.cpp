#include <Rtrc/Renderer/GBufferBinding.h>
#include <Rtrc/Renderer/PathTracer/PathTracer.h>
#include <Rtrc/Renderer/Scene/RenderCamera.h>
#include <Rtrc/Renderer/Utility/PcgStateTexture.h>

RTRC_RENDERER_BEGIN

void PathTracer::Render(
    RG::RenderGraph &renderGraph,
    RenderCamera    &camera,
    const GBuffers  &gbuffers,
    bool             clearBeforeRender) const
{
    RTRC_RG_SCOPED_PASS_GROUP(renderGraph, "PathTracing");

    auto &scene = camera.GetScene();
    PerCameraData &perCameraData = camera.GetPathTracingData();

    const Vector2u framebufferSize = gbuffers.normal->GetSize();
    auto rngState = Prepare2DPcgStateTexture(renderGraph, resources_, perCameraData.rngState, framebufferSize);

    // =================== Trace ===================

    auto traceResult = renderGraph.CreateTexture(RHI::TextureDesc
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
            "Clear IndirectDiffuse Output", traceResult, Vector4f(0, 0, 0, 0));
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

    auto tracePass = renderGraph.CreatePass("Trace");
    DeclareGBufferUses<TracePassGroup>(tracePass, gbuffers, RHI::PipelineStage::ComputeShader);
    tracePass->Use(camera.GetAtmosphereData().S,  RG::CS_Texture);
    tracePass->Use(scene.GetRGTlas(),             RG::CS_ReadAS);
    tracePass->Use(scene.GetRGInstanceBuffer(),   RG::CS_StructuredBuffer);
    tracePass->Use(rngState,                      RG::CS_RWTexture);
    tracePass->Use(traceResult,                   RG::CS_RWTexture_WriteOnly);
    tracePass->SetCallback([gbuffers, traceResult, rngState, &scene, &camera, framebufferSize, this]
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
        passData.Output           = traceResult;
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

    // =================== Temporal filter ===================

    std::swap(perCameraData.prev, perCameraData.curr);
    if(perCameraData.prev && perCameraData.prev->GetSize() != framebufferSize)
    {
        perCameraData.prev = nullptr;
    }

    RG::TextureResource *prev, *curr;
    if(!perCameraData.curr || perCameraData.curr->GetSize() != framebufferSize)
    {
        const RHI::TextureDesc desc =
        {
            .dim    = RHI::TextureDimension::Tex2D,
            .format = RHI::Format::R32G32B32A32_Float,
            .width  = framebufferSize.x,
            .height = framebufferSize.y,
            .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess
        };
        perCameraData.curr = device_->CreatePooledTexture(desc);
        perCameraData.prev = device_->CreatePooledTexture(desc);
        perCameraData.curr->SetName("PathTracer-TemporalFilteredImage-A");
        perCameraData.prev->SetName("PathTracer-TemporalFilteredImage-B");
        prev = renderGraph.RegisterTexture(perCameraData.prev);
        curr = renderGraph.RegisterTexture(perCameraData.curr);
        renderGraph.CreateClearRWTexture2DPass("ClearPrevResult", prev, Vector4f(0, 0, 0, 0));
    }
    else
    {
        prev = renderGraph.RegisterTexture(perCameraData.prev);
        curr = renderGraph.RegisterTexture(perCameraData.curr);
    }

    rtrc_group(TemporalFilterPassGroup, CS)
    {
        rtrc_define(Texture2D, CurrDepth);
        rtrc_define(Texture2D, PrevDepth);

        rtrc_define(ConstantBuffer<CameraConstantBuffer>, CurrCamera);
        rtrc_define(ConstantBuffer<CameraConstantBuffer>, PrevCamera);

        rtrc_define(Texture2D,   History);
        rtrc_define(Texture2D,   TraceResult);
        rtrc_define(RWTexture2D, Resolved);

        rtrc_uniform(uint2, resolution);
    };

    auto temporalFilterPass = renderGraph.CreatePass("TemporalFilter");
    temporalFilterPass->Use(gbuffers.currDepth, RG::CS_Texture);
    temporalFilterPass->Use(gbuffers.prevDepth, RG::CS_Texture);
    temporalFilterPass->Use(prev,               RG::CS_Texture);
    temporalFilterPass->Use(traceResult,        RG::CS_Texture);
    temporalFilterPass->Use(curr,               RG::CS_RWTexture_WriteOnly);
    temporalFilterPass->SetCallback([gbuffers, prev, traceResult, curr, &camera, this]
    {
        auto material = resources_->GetBuiltinMaterial(BuiltinMaterial::PathTracing);
        auto shader = material->GetPassByIndex(PassIndex_TemporalFilter)->GetShader().get();

        TemporalFilterPassGroup passData;
        passData.PrevDepth   = gbuffers.prevDepth;
        passData.CurrDepth   = gbuffers.currDepth;
        passData.CurrCamera  = camera.GetCameraCBuffer();
        passData.PrevCamera  = camera.GetPrevCameraCBuffer();
        passData.History     = prev;
        passData.TraceResult = traceResult;
        passData.Resolved    = curr;
        passData.resolution  = traceResult->GetSize();
        auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

        auto &commandBuffer = RG::GetCurrentCommandBuffer();
        commandBuffer.BindComputePipeline(shader->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(traceResult->GetSize());
    });

    // =================== Spatial filter ===================

    rtrc_group(SpatialFilterPassGroup, CS)
    {
        rtrc_inline(GBufferBindings_NormalDepth, gbuffers);
        rtrc_define(Texture2D, In);
        rtrc_define(RWTexture2D, Out);
        rtrc_uniform(uint2, resolution);
        rtrc_uniform(float4x4, cameraToClip);
    };

    auto AddSpatialFilterPass = [&]<bool IsDirectionY>(RG::TextureResource *in, RG::TextureResource *out)
    {
        auto pass = renderGraph.CreatePass("SpatialFilter");
        DeclareGBufferUses<SpatialFilterPassGroup>(pass, gbuffers, RHI::PipelineStage::ComputeShader);
        pass->Use(in, RG::CS_Texture);
        pass->Use(out, RG::CS_RWTexture_WriteOnly);
        pass->SetCallback([gbuffers, in, out, &camera, this]
        {
            KeywordContext keywords;
            keywords.Set(RTRC_KEYWORD(IS_FILTER_DIRECTION_Y), IsDirectionY ? 1 : 0);
            auto material = resources_->GetBuiltinMaterial(BuiltinMaterial::PathTracing);
            auto shader = material->GetPassByIndex(PassIndex_SpatialFilter)->GetShader(keywords).get();

            SpatialFilterPassGroup passData;
            FillBindingGroupGBuffers(passData, gbuffers);
            passData.In           = in;
            passData.Out          = out;
            passData.resolution   = in->GetSize();
            passData.cameraToClip = camera.GetCameraRenderData().cameraToClip;
            auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

            auto &commandBuffer = RG::GetCurrentCommandBuffer();
            commandBuffer.BindComputePipeline(shader->GetComputePipeline());
            commandBuffer.BindComputeGroup(0, passGroup);
            commandBuffer.DispatchWithThreadCount(in->GetSize());
        });
    };

    auto pathTracingFilterTemp = renderGraph.CreateTexture(traceResult->GetDesc(), "TempRT-PathTracerFiltering");
    AddSpatialFilterPass.operator()<false>(curr, pathTracingFilterTemp);
    AddSpatialFilterPass.operator()<true>(pathTracingFilterTemp, traceResult);

    assert(!perCameraData.indirectDiffuse);
    perCameraData.indirectDiffuse = traceResult;
}

void PathTracer::ClearFrameData(PerCameraData &data) const
{
    data.indirectDiffuse = nullptr;
}

RTRC_RENDERER_END
