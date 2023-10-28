#include <Rtrc/Renderer/GBufferBinding.h>
#include <Rtrc/Renderer/PathTracer/PathTracer.h>
#include <Rtrc/Renderer/GPUScene/RenderCamera.h>
#include <Rtrc/Renderer/Utility/PcgStateTexture.h>

#include <Rtrc/Renderer/PathTracer/Shader/PathTracing.shader.outh>

RTRC_RENDERER_BEGIN

void PathTracer::Render(
    RG::RenderGraph &renderGraph,
    RenderCamera    &camera,
    bool             clearBeforeRender) const
{
    RTRC_RG_SCOPED_PASS_GROUP(renderGraph, "PathTracing");

    auto &scene = camera.GetScene();
    PerCameraData &perCameraData = camera.GetPathTracingData();

    auto &gbuffers = camera.GetGBuffers();
    const Vector2u framebufferSize = gbuffers.currNormal->GetSize();
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

    using TraceGroup = StaticShaderInfo<"PathTracing/Trace">::Pass;

    TraceGroup tracePassData;
    FillBindingGroupGBuffers(tracePassData, gbuffers);
    tracePassData.Camera                    = camera.GetCameraCBuffer();
    tracePassData.SkyLut                    = camera.GetAtmosphereData().S;
    tracePassData.OpaqueScene_Tlas          = scene.GetTlas();
    tracePassData.OpaqueScene_TlasInstances = scene.GetInstanceBuffer()->GetStructuredSrv(sizeof(RenderScene::TlasInstance));
    tracePassData.RngState                  = rngState;
    tracePassData.Output                    = traceResult;
    tracePassData.Output.writeOnly          = true;
    tracePassData.outputResolution          = framebufferSize;
    tracePassData.maxDepth                  = 5;
    renderGraph.CreateComputePassWithThreadCount(
        "Trace",
        resources_->GetMaterialManager()->GetCachedShader<"PathTracing/Trace">(),
        framebufferSize,
        tracePassData,
        scene.GetBindlessGeometryBuffers(),
        scene.GetBindlessTextures());

    // =================== Temporal filter ===================

    std::swap(perCameraData.prev, perCameraData.curr);
    assert((perCameraData.prev != nullptr) == (perCameraData.curr != nullptr));
    if(perCameraData.curr && perCameraData.curr->GetSize() != framebufferSize)
    {
        perCameraData.curr = nullptr;
    }

    RG::TextureResource *prev, *curr;
    if(!perCameraData.curr)
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

    using TemporalFilterGroup = StaticShaderInfo<"PathTracing/TemporalFilter">::Pass;
    
    TemporalFilterGroup temporalPassData;
    temporalPassData.PrevDepth          = gbuffers.prevDepth ? gbuffers.prevDepth : gbuffers.currDepth;
    temporalPassData.CurrDepth          = gbuffers.currDepth;
    temporalPassData.CurrCamera         = camera.GetCameraCBuffer();
    temporalPassData.PrevCamera         = camera.GetPreviousCameraCBuffer();
    temporalPassData.History            = prev;
    temporalPassData.TraceResult        = traceResult;
    temporalPassData.Resolved           = curr;
    temporalPassData.Resolved.writeOnly = true;
    temporalPassData.resolution         = traceResult->GetSize();
    renderGraph.CreateComputePassWithThreadCount(
        "TemporalFilter",
        resources_->GetMaterialManager()->GetCachedShader<"PathTracing/TemporalFilter">(),
        traceResult->GetSize(),
        temporalPassData);

    // =================== Spatial filter ===================

    using SpatialFilterGroup = StaticShaderInfo<"PathTracing/SpatialFilter">::Variant<false>::Pass;

    auto AddSpatialFilterPass = [&](RG::TextureResource *in, RG::TextureResource *out, bool isDirectionY)
    {
        FastKeywordContext keywords;
        keywords.Set(RTRC_FAST_KEYWORD(IS_FILTER_DIRECTION_Y), isDirectionY ? 1 : 0);
        auto shader = resources_->GetMaterialManager()
                                ->GetCachedShaderTemplate<"PathTracing/SpatialFilter">()
                                ->GetVariant(keywords);

        SpatialFilterGroup passData;
        FillBindingGroupGBuffers(passData, gbuffers);
        passData.In            = in;
        passData.Out           = out;
        passData.Out.writeOnly = true;
        passData.resolution    = in->GetSize();
        passData.cameraToClip  = camera.GetCameraData().cameraToClip;

        renderGraph.CreateComputePassWithThreadCount(
            "SpatialFilter",
            shader,
            in->GetSize(),
            passData);
    };

    auto pathTracingFilterTemp = renderGraph.CreateTexture(traceResult->GetDesc(), "TempRT-PathTracerFiltering");
    AddSpatialFilterPass(curr, pathTracingFilterTemp, false);
    AddSpatialFilterPass(pathTracingFilterTemp, traceResult, true);

    assert(!perCameraData.indirectDiffuse);
    perCameraData.indirectDiffuse = traceResult;
}

void PathTracer::ClearFrameData(PerCameraData &data) const
{
    data.indirectDiffuse = nullptr;
}

RTRC_RENDERER_END
