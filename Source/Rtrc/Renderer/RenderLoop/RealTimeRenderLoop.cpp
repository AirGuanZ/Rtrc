#include <Rtrc/Renderer/PathTracer/PathTracer.h>
#include <Rtrc/Renderer/ReSTIR/ReSTIR.h>
#include <Rtrc/Renderer/RenderLoop/RealTimeRenderLoop.h>

RTRC_RENDERER_BEGIN

RealTimeRenderLoop::RealTimeRenderLoop(ObserverPtr<ResourceManager> resources, ObserverPtr<BindlessTextureManager> bindlessTextures)
    : device_(resources->GetDevice())
    , resources_(resources)
    , bindlessTextures_(bindlessTextures)
    , frameIndex_(1)
    , isSwapchainInvalid_(false)
{
    imguiRenderer_ = MakeBox<ImGuiRenderer>(device_);
    renderGraphExecuter_ = MakeBox<RG::Executer>(device_);
    transientCBufferAllocator_ = MakeBox<TransientConstantBufferAllocator>(*device_);

    cachedMeshes_ = MakeBox<MeshRenderingCacheManager>(device_);
    cachedMaterials_ = MakeBox<MaterialRenderingCacheManager>();
    renderScenes_ = MakeBox<RenderSceneManager>(resources_, cachedMaterials_, cachedMeshes_, bindlessTextures_);
    renderCameras_ = MakeBox<RenderCameraManager>(device_, renderScenes_);

    gbufferPass_ = MakeBox<GBufferPass>(device_);
    pathTracer_ = MakeBox<PathTracer>(resources_);
    gbufferVisualizer_ = MakeBox<GBufferVisualizer>(resources_);
    restir_ = MakeBox<ReSTIR>(resources_);
    deferredLighting_ = MakeBox<DeferredLighting>(resources_);
}

void RealTimeRenderLoop::BeginRenderLoop()
{
    device_->BeginRenderLoop();
}

void RealTimeRenderLoop::EndRenderLoop()
{
    device_->EndRenderLoop();
}

void RealTimeRenderLoop::SetRenderSettings(const RenderSettings &settings)
{
    renderSettings_ = settings;
}

void RealTimeRenderLoop::ResizeFramebuffer(uint32_t width, uint32_t height)
{
    if(width > 0 && height > 0)
    {
        device_->GetRawDevice()->WaitIdle();
        device_->RecreateSwapchain();
        isSwapchainInvalid_ = false;
    }
}

void RealTimeRenderLoop::RenderFrame(const FrameInput &frame)
{
    if(isSwapchainInvalid_)
    {
        return;
    }
    if(!device_->BeginFrame(false))
    {
        isSwapchainInvalid_ = true;
        return;
    }
    renderGraphExecuter_->NewFrame();
    ++frameIndex_;
    frameTimer_.BeginFrame();
    RenderFrameImpl(frame);
    if(!device_->Present())
    {
        isSwapchainInvalid_ = true;
    }
}

void RealTimeRenderLoop::RenderFrameImpl(const FrameInput &frame)
{
    LinearAllocator linearAllocator;
    transientCBufferAllocator_->NewBatch();

    auto renderGraph = device_->CreateRenderGraph();
    auto swapchainImage = renderGraph->RegisterSwapchainTexture(device_->GetSwapchain());
    auto framebuffer = renderGraph->CreateTexture(RHI::TextureDesc
    {
        .dim         = RHI::TextureDimension::Tex2D,
        .format      = RHI::Format::B8G8R8A8_UNorm,
        .width       = UpAlignTo(swapchainImage->GetWidth(), 4u),
        .height      = UpAlignTo(swapchainImage->GetHeight(), 4u),
        .arraySize   = 1,
        .mipLevels   = 1,
        .sampleCount = 1,
        .usage       = RHI::TextureUsage::ShaderResource |
                       RHI::TextureUsage::UnorderAccess |
                       RHI::TextureUsage::RenderTarget,
    }, "Framebuffer");

    // ============= Common scene/camera updates =============

    cachedMeshes_->Update(*frame.scene, *frame.camera);
    cachedMaterials_->Update(*frame.scene);
    auto buildBlasPass = cachedMeshes_->Build(*renderGraph, 5, 100000);

    auto &renderScene = renderScenes_->GetRenderScene(*frame.scene);
    auto &renderCamera = renderCameras_->GetRenderCamera(*frame.scene, *frame.camera);

    RenderScene::Config renderSceneConfig;
    renderSceneConfig.opaqueTlas = true;
    renderScene.FrameUpdate(renderSceneConfig, *renderGraph);
    RTRC_SCOPE_EXIT{ renderScene.ClearFrameData(); };

    renderCamera.Update(*cachedMeshes_, *cachedMaterials_, linearAllocator);

    // Blas buffers are read-only after initialized and not tracked by render graph,
    // so we need to manually add dependency.
    if(buildBlasPass && renderScene.GetBuildTlasPass())
    {
        Connect(buildBlasPass, renderScene.GetBuildTlasPass());
    }

    // ============= GBuffers =============

    renderCamera.CreateGBuffers(renderGraph, framebuffer->GetSize());
    RTRC_SCOPE_EXIT{ renderCamera.ClearGBuffers(); };
    
    gbufferPass_->Render(renderCamera, bindlessTextures_->GetBindingGroup(), *renderGraph);

    // ============= Resolve depth =============

    renderCamera.ResolveDepthTexture(*renderGraph);

    // ============= Atmosphere =============

    auto &atmosphere = renderScene.GetRenderAtmosphere();
    auto &cameraAtmosphereData = renderCamera.GetAtmosphereData();
    atmosphere.Render(
        *renderGraph,
        frame.scene->GetSky().GetSunDirection(),
        frame.scene->GetSky().GetSunColor() * frame.scene->GetSky().GetSunIntensity(),
        1000.0f,
        frameTimer_.GetDeltaSecondsF(),
        cameraAtmosphereData);
    RTRC_SCOPE_EXIT{ atmosphere.ClearPerCameraFrameData(cameraAtmosphereData); };
    
    // ============= Indirect diffuse =============

    const bool visIndirectDiffuse = renderSettings_.visualizationMode == VisualizationMode::IndirectDiffuse;
    const bool renderIndirectDiffuse = visIndirectDiffuse || renderSettings_.enableIndirectDiffuse;
    if(renderIndirectDiffuse)
    {
        pathTracer_->Render(*renderGraph, renderCamera, visIndirectDiffuse);
    }
    auto indirectDiffuse = renderCamera.GetPathTracingData().indirectDiffuse;
    RTRC_SCOPE_EXIT{ pathTracer_->ClearFrameData(renderCamera.GetPathTracingData()); };

    // ============= ReSTIR =============

    ReSTIR::Settings restirSettings;
    restirSettings.M                           = renderSettings_.ReSTIR_M;
    restirSettings.maxM                        = renderSettings_.ReSTIR_MaxM;
    restirSettings.N                           = renderSettings_.ReSTIR_N;
    restirSettings.radius                      = renderSettings_.ReSTIR_Radius;
    restirSettings.enableTemporalReuse         = renderSettings_.ReSTIR_EnableTemporalReuse;
    restirSettings.svgfTemporalFilterAlpha     = renderSettings_.ReSTIR_SVGFTemporalFilterAlpha;
    restirSettings.svgfSpatialFilterIterations = renderSettings_.ReSTIR_SVGFSpatialFilterIterations;
    restir_->SetSettings(restirSettings);
    restir_->Render(renderCamera, *renderGraph);
    RTRC_SCOPE_EXIT{ restir_->ClearFrameData(renderCamera.GetReSTIRData()); };

    // ============= Lighting =============

    deferredLighting_->Render(
        renderCamera, 
        renderGraph,
        renderCamera.GetReSTIRData().directIllum, 
        renderCamera.GetAtmosphereData().S, 
        framebuffer);
    
    // ============= Blit =============

    const bool blitUsePointSampling = framebuffer->GetSize() == swapchainImage->GetSize();
    if(renderSettings_.visualizationMode == VisualizationMode::IndirectDiffuse)
    {
        renderGraph->CreateBlitTexture2DPass(
            "DisplayIndirectDiffuse", indirectDiffuse, swapchainImage, blitUsePointSampling, 1 / 2.2f);
    }
    else if(renderSettings_.visualizationMode == VisualizationMode::Normal)
    {
        gbufferVisualizer_->Render(
            GBufferVisualizer::Mode::Normal, *renderGraph, renderCamera.GetGBuffers(), framebuffer);
        renderGraph->CreateBlitTexture2DPass(
            "BlitToSwapchain", framebuffer, swapchainImage, blitUsePointSampling);
    }
    else if(renderSettings_.visualizationMode == VisualizationMode::ReSTIRDirectIllumination)
    {
        renderGraph->CreateBlitTexture2DPass(
            "BlitToSwapchain", renderCamera.GetReSTIRData().directIllum, swapchainImage, blitUsePointSampling, 1 / 2.2f);
    }
    else
    {
        renderGraph->CreateBlitTexture2DPass(
            "BlitToSwapchain", framebuffer, swapchainImage, blitUsePointSampling);
    }
    
    // ============= ImGui =============

    imguiRenderer_->Render(frame.imguiDrawData, swapchainImage, renderGraph.get());

    // ============= Execution =============

    transientCBufferAllocator_->Flush();
    renderGraph->SetCompleteFence(device_->GetFrameFence());
    renderGraphExecuter_->Execute(renderGraph);
}

RTRC_RENDERER_END
