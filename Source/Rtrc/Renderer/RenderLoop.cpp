#include <Rtrc/Renderer/RenderLoop.h>
#include <Rtrc/Utility/Enumerate.h>
#include <Rtrc/Utility/Thread.h>

RTRC_RENDERER_BEGIN

RenderLoop::RenderLoop(
    const Config                       &config,
    ObserverPtr<Device>                 device,
    ObserverPtr<BuiltinResourceManager> builtinResources,
    ObserverPtr<BindlessTextureManager> bindlessTextures)
    : config_          (config)
    , hasException_    (false)
    , device_          (device)
    , builtinResources_(builtinResources)
    , bindlessTextures_(bindlessTextures)
    , frameIndex_      (1)
    , renderMeshes_    ({ config.rayTracing }, device)
    , renderScene_     ({ config.rayTracing }, device, builtinResources, bindlessTextures)
{
    imguiRenderer_       = MakeBox<ImGuiRenderer>(device_);
    renderGraphExecuter_ = MakeBox<RG::Executer>(device_);

    transientConstantBufferAllocator_ = MakeBox<TransientConstantBufferAllocator>(*device);
    
    gbufferPass_          = MakeBox<GBufferPass>(device_);
    deferredLightingPass_ = MakeBox<DeferredLightingPass>(device_, builtinResources_);
    shadowMaskPass_       = MakeBox<ShadowMaskPass>(device_, builtinResources_);
    pathTracer_           = MakeBox<PathTracer>(device_, builtinResources_);

    device_->BeginRenderLoop();
    if(config_.mode == Mode::Threaded)
    {
        renderThread_ = std::jthread(&RenderLoop::RenderThreadEntry, this);
    }
}

RenderLoop::~RenderLoop()
{
    if(config_.mode == Mode::Threaded)
    {
        assert(renderThread_.joinable());
        renderThread_.join();
        renderCommandQueue_.clear();
    }
}

void RenderLoop::AddCommand(RenderCommand command)
{
    if(config_.mode == Mode::Threaded)
    {
        renderCommandQueue_.push(std::move(command));
        return;
    }

    assert(continueRenderLoop_);
    command.Match(
        [&](const RenderCommand_ResizeFramebuffer &resize)
        {
            RTRC_SCOPE_EXIT{ resize.finishSemaphore->release(); };
            if(resize.width > 0 && resize.height > 0)
            {
                device_->GetRawDevice()->WaitIdle();
                device_->RecreateSwapchain();
                isSwapchainInvalid_ = false;
            }
        },
        [&](const RenderCommand_RenderStandaloneFrame &frame)
        {
            RTRC_SCOPE_EXIT{ frame.finishSemaphore->release(); };
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
            RenderStandaloneFrame(frame);
            if(!device_->Present())
            {
                isSwapchainInvalid_ = true;
            }
        },
        [&](const RenderCommand_Exit &)
        {
            continueRenderLoop_ = false;
            device_->EndRenderLoop();
        });
}

const BindlessTextureEntry *RenderLoop::ExtractAlbedoTextureEntry(const MaterialInstance::SharedRenderingData *material)
{
    return material->GetPropertySheet().GetBindlessTextureEntry(RTRC_MATERIAL_PROPERTY_NAME(AlbedoTextureIndex));
}

void RenderLoop::RenderThreadEntry()
{
    SetThreadIndentifier(ThreadUtility::ThreadIdentifier::Render);

    auto DoWithExceptionHandling = [&]<typename F>(const F &f)
    {
        if(config_.handleCrossThreadException)
        {
            try
            {
                f();
            }
            catch(...)
            {
                hasException_ = true;
                exceptionPtr_ = std::current_exception();
                continueRenderLoop_ = false;
            }
        }
        else
        {
            f();
        }
    };

    RTRC_SCOPE_EXIT{ device_->EndRenderLoop(); };

    frameTimer_.Restart();
    while(continueRenderLoop_)
    {
        RenderCommand nextRenderCommand;
        if(renderCommandQueue_.try_pop(nextRenderCommand))
        {
            nextRenderCommand.Match(
                [&](const RenderCommand_ResizeFramebuffer &resize)
                {
                    RTRC_SCOPE_EXIT{ resize.finishSemaphore->release(); };
                    if(resize.width > 0 && resize.height > 0)
                    {
                        DoWithExceptionHandling([&]
                        {
                            device_->GetRawDevice()->WaitIdle();
                            device_->RecreateSwapchain();
                            isSwapchainInvalid_ = false;
                        });
                    }
                },
                [&](const RenderCommand_RenderStandaloneFrame &frame)
                {
                    RTRC_SCOPE_EXIT{ frame.finishSemaphore->release(); };
                    DoWithExceptionHandling([&]
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
                        RenderStandaloneFrame(frame);
                        if(!device_->Present())
                        {
                            isSwapchainInvalid_ = true;
                        }
                    });
                },
                [&](const RenderCommand_Exit &)
                {
                    continueRenderLoop_ = false;
                });
        }
    }
}

void RenderLoop::RenderStandaloneFrame(const RenderCommand_RenderStandaloneFrame &frame)
{
    LinearAllocator linearAllocator;
    transientConstantBufferAllocator_->NewBatch();

    auto renderGraph = device_->CreateRenderGraph();
    auto swapchainImage = renderGraph->RegisterSwapchainTexture(device_->GetSwapchain());
    auto framebuffer = renderGraph->CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::B8G8R8A8_UNorm,
        .width                = swapchainImage->GetWidth(),
        .height               = swapchainImage->GetHeight(),
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::ShaderResource |
                                RHI::TextureUsage::UnorderAccess |
                                RHI::TextureUsage::RenderTarget,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    }, "Framebuffer");
    
    // ============= Update meshes =============

    renderMeshes_.Update(frame);
    constexpr int MAX_BLAS_BUILD_COUNT = 5;
    constexpr int MAX_BLAS_BUILD_PRIMITIVE_COUNT = 1e5;
    renderMeshes_.BuildBlasForMeshes(*renderGraph, MAX_BLAS_BUILD_COUNT, MAX_BLAS_BUILD_PRIMITIVE_COUNT);
    RTRC_SCOPE_EXIT{ renderMeshes_.ClearFrameData(); };

    // ============= Update materials =============

    renderMaterials_.UpdateCachedMaterialData(frame);

    // ============= Update scene =============

    renderScene_.Update(
        frame, *transientConstantBufferAllocator_,
        renderMeshes_, renderMaterials_, *renderGraph, linearAllocator);
    RTRC_SCOPE_EXIT{ renderScene_.ClearFrameData(); };
    RenderCamera *renderCamera = renderScene_.GetRenderCamera(frame.camera.originalId);
    
    // Blas buffers are read-only after initialized and not tracked by render graph,
    // so we need to manually add dependency.

    if(renderMeshes_.GetRGBuildBlasPass() && renderScene_.GetRGBuildTlasPass())
    {
        Connect(renderMeshes_.GetRGBuildBlasPass(), renderScene_.GetRGBuildTlasPass());
    }
    
    // ============= GBuffers =============

    GBuffers gbuffers = gbufferPass_->Render(
        *renderCamera, frame.bindlessTextureGroup, *renderGraph, framebuffer->GetSize());

    // ============= Atmosphere =============

    auto &renderAtmosphere = renderScene_.GetRenderAtmosphere();
    auto &cameraAtmosphere = renderCamera->GetAtmosphereData();
    renderAtmosphere.Render(
        *renderGraph,
        frame.scene->GetSunDirection(),
        frame.scene->GetSunColor() * frame.scene->GetSunIntensity(),
        1000.0f,
        frameTimer_.GetDeltaSecondsF(),
        cameraAtmosphere);
    RTRC_SCOPE_EXIT{ renderAtmosphere.ClearPerCameraFrameData(cameraAtmosphere); };
    auto skyLut = cameraAtmosphere.S;

    // ============= Indirect diffuse =============

    const bool visIndirectDiffuse = frame.renderSettings.visualizationMode == VisualizationMode::IndirectDiffuse;
    pathTracer_->Render(*renderGraph, *renderCamera, gbuffers, framebuffer->GetSize(), visIndirectDiffuse);
    RTRC_SCOPE_EXIT{ pathTracer_->ClearFrameData(renderCamera->GetPathTracingData()); };

    // ============= Deferred lighting =============
    
    deferredLightingPass_->Render(
        *renderCamera, gbuffers, skyLut, *renderGraph, framebuffer);

    // ============= Blit =============

    if(frame.renderSettings.visualizationMode == VisualizationMode::IndirectDiffuse)
    {
        renderGraph->CreateBlitTexture2DPass(
            "Visualize indirect diffuse",
            renderCamera->GetPathTracingData().indirectDiffuse,
            swapchainImage,
            true, 1 / 2.2f);
    }
    else
    {
        renderGraph->CreateBlitTexture2DPass(
            "BlitToSwapchain",
            framebuffer,
            swapchainImage,
            true, 1.0f);
    }

    // ============= ImGui =============

    imguiRenderer_->Render(frame.imguiDrawData.get(), swapchainImage, renderGraph.get());
    
    // ============= Execution =============

    transientConstantBufferAllocator_->Flush();
    
    renderGraph->SetCompleteFence(device_->GetFrameFence());
    renderGraphExecuter_->Execute(renderGraph);
}

RTRC_RENDERER_END
