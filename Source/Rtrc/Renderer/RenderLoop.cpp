#include <Rtrc/Renderer/RenderLoop.h>

RTRC_RENDERER_BEGIN

RenderLoop::RenderLoop(ObserverPtr<ResourceManager> resources, ObserverPtr<BindlessTextureManager> bindlessTextures)
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
}

void RenderLoop::BeginRenderLoop()
{
    device_->BeginRenderLoop();
}

void RenderLoop::EndRenderLoop()
{
    device_->EndRenderLoop();
}

void RenderLoop::SetRenderSettings(const RenderSettings &settings)
{
    renderSettings_ = settings;
}

void RenderLoop::ResizeFramebuffer(uint32_t width, uint32_t height)
{
    if(width > 0 && height > 0)
    {
        device_->GetRawDevice()->WaitIdle();
        device_->RecreateSwapchain();
        isSwapchainInvalid_ = false;
    }
}

void RenderLoop::RenderFrame(const FrameInput &frame)
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
    ++frameIndex_;
    frameTimer_.BeginFrame();
    RenderFrameImpl(frame);
    if(!device_->Present())
    {
        isSwapchainInvalid_ = true;
    }
}

void RenderLoop::RenderFrameImpl(const FrameInput &frame)
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
    renderSceneConfig.opaqueTlas = renderSettings_.enableRayTracing;
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

    auto gbuffers = gbufferPass_->Render(
        renderCamera, bindlessTextures_->GetBindingGroup(), *renderGraph, framebuffer->GetSize());

    // ============= Resolve depth =============

    renderCamera.UpdateDepthTexture(*renderGraph, gbuffers);

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
        pathTracer_->Render(*renderGraph, renderCamera, gbuffers, visIndirectDiffuse);
    }
    auto indirectDiffuse = renderCamera.GetPathTracingData().indirectDiffuse;
    RTRC_SCOPE_EXIT{ pathTracer_->ClearFrameData(renderCamera.GetPathTracingData()); };

    gbufferVisualizer_->Render(GBufferVisualizer::Mode::Normal, *renderGraph, gbuffers, framebuffer);

    // ============= Blit =============

    if(renderSettings_.visualizationMode == VisualizationMode::IndirectDiffuse)
    {
        renderGraph->CreateBlitTexture2DPass("DisplayIndirectDiffuse", indirectDiffuse, swapchainImage, true, 1 / 2.2f);
    }
    else
    {
        const bool usePointSampling = framebuffer->GetSize() == swapchainImage->GetSize();
        renderGraph->CreateBlitTexture2DPass("BlitToSwapchain", framebuffer, swapchainImage, usePointSampling);
    }
    
    // ============= ImGui =============

    imguiRenderer_->Render(frame.imguiDrawData, swapchainImage, renderGraph.get());

    // ============= Execution =============

    transientCBufferAllocator_->Flush();
    renderGraph->SetCompleteFence(device_->GetFrameFence());
    renderGraphExecuter_->Execute(renderGraph);
}

RTRC_RENDERER_END
