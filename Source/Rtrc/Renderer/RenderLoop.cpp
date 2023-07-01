#include <Rtrc/Renderer/RenderLoop.h>
#include <Rtrc/Utility/Enumerate.h>
#include <Rtrc/Utility/Thread.h>

RTRC_RENDERER_BEGIN

RenderLoop::RenderLoop(
    const Config                             &config,
    ObserverPtr<Device>                       device,
    ObserverPtr<const BuiltinResourceManager> builtinResources,
    ObserverPtr<BindlessTextureManager>       bindlessTextures)
    : config_          (config)
    , hasException_    (false)
    , device_          (device)
    , builtinResources_(builtinResources)
    , bindlessTextures_(bindlessTextures)
    , frameIndex_      (1)
    , meshManager_     ({ config.rayTracing }, device)
    , cachedScene_     ({ config.rayTracing }, device)
{
    imguiRenderer_       = MakeBox<ImGuiRenderer>(device_);
    renderGraphExecuter_ = MakeBox<RG::Executer>(device_);

    transientConstantBufferAllocator_ = MakeBox<TransientConstantBufferAllocator>(*device);

    atmospherePass_       = MakeBox<PhysicalAtmospherePass>(device_, builtinResources_);
    gbufferPass_          = MakeBox<GBufferPass>(device_);
    deferredLightingPass_ = MakeBox<DeferredLightingPass>(device_, builtinResources_);
    shadowMaskPass_       = MakeBox<ShadowMaskPass>(device_, builtinResources_);

    renderThread_ = std::jthread(&RenderLoop::RenderThreadEntry, this);
}

RenderLoop::~RenderLoop()
{
    assert(renderThread_.joinable());
    renderThread_.join();
    renderCommandQueue_.clear();
}

void RenderLoop::AddCommand(RenderCommand command)
{
    renderCommandQueue_.push(std::move(command));
}

const BindlessTextureEntry *RenderLoop::ExtractAlbedoTextureEntry(const MaterialInstance::SharedRenderingData *material)
{
    return material->GetPropertySheet().GetBindlessTextureEntry(RTRC_MATERIAL_PROPERTY_NAME(AlbedoTextureIndex));
}

void RenderLoop::RenderThreadEntry()
{
    SetThreadIndentifier(ThreadUtility::ThreadIdentifier::Render);

    bool isSwapchainInvalid = false;
    bool continueRenderLoop = true;

    device_->BeginRenderLoop();
    RTRC_SCOPE_EXIT{ device_->EndRenderLoop(); };

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
                continueRenderLoop = false;
            }
        }
        else
        {
            f();
        }
    };

    frameTimer_.Restart();
    while(continueRenderLoop)
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
                            isSwapchainInvalid = false;
                        });
                    }
                },
                [&](const RenderCommand_RenderStandaloneFrame &frame)
                {
                    RTRC_SCOPE_EXIT{ frame.finishSemaphore->release(); };
                    DoWithExceptionHandling([&]
                    {
                        if(isSwapchainInvalid)
                        {
                            return;
                        }
                        if(!device_->BeginFrame(false))
                        {
                            isSwapchainInvalid = true;
                            return;
                        }
                        ++frameIndex_;
                        frameTimer_.BeginFrame();
                        RenderStandaloneFrame(frame);
                        if(!device_->Present())
                        {
                            isSwapchainInvalid = true;
                        }
                    });
                },
                [&](const RenderCommand_Exit &)
                {
                    continueRenderLoop = false;
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

    meshManager_.Update(frame);
    constexpr int MAX_BLAS_BUILD_COUNT = 5;
    constexpr int MAX_BLAS_BUILD_PRIMITIVE_COUNT = 1e5;
    meshManager_.BuildBlasForMeshes(*renderGraph, MAX_BLAS_BUILD_COUNT, MAX_BLAS_BUILD_PRIMITIVE_COUNT);
    RTRC_SCOPE_EXIT{ meshManager_.ClearFrameData(); };

    // ============= Update materials =============

    materialManager_.UpdateCachedMaterialData(frame);

    // ============= Update scene =============

    cachedScene_.Update(
        frame, *transientConstantBufferAllocator_,
        meshManager_, materialManager_, *renderGraph, linearAllocator);
    RTRC_SCOPE_EXIT{ cachedScene_.ClearFrameData(); };

    RenderSceneCamera *renderSceneCamera = cachedScene_.GetSceneCameraRenderingData(frame.camera.originalId);
    assert(renderSceneCamera);

    // Blas buffers are not tracked by render graph, so we need to manually add dependency.

    if(meshManager_.GetRGBuildBlasPass() && cachedScene_.GetRGBuildTlasPass())
    {
        Connect(meshManager_.GetRGBuildBlasPass(), cachedScene_.GetRGBuildTlasPass());
    }
    
    // ============= GBuffers =============

    GBuffers gbuffers = gbufferPass_->Render(
        *renderSceneCamera, frame.bindlessTextureGroup, *renderGraph, framebuffer->GetSize());

    // ============= Atmosphere =============

    atmospherePass_->SetProperties(frame.scene->GetAtmosphere());
    auto skyLut = atmospherePass_->Render(
        *renderGraph, 
        frame.scene->GetSunDirection(), 
        frame.scene->GetSunColor() * frame.scene->GetSunIntensity(),
        1000.0f,
        frameTimer_.GetDeltaSecondsF(),
        renderSceneCamera->GetCachedAtmosphereData()).skyLut;

    // ============= Shadow mask =============
    
    int shadowMaskLightIndex = -1;
    for(auto &&[i, light] : Enumerate(renderSceneCamera->GetLights()))
    {
        if(light->GetFlags().Contains(LightDetail::FlagBit::EnableRayTracedShadow))
        {
            if(shadowMaskLightIndex < 0)
            {
                shadowMaskLightIndex = static_cast<int>(i);
            }
            else
            {
                throw Exception("Only one light can enable ray traced shadow mask");
            }
        }
    }
    auto shadowMask = shadowMaskPass_->Render(
        *renderSceneCamera,
        renderSceneCamera->GetCachedScene().GetRenderLights().GetMainLight(),
        frame.renderSettings.enableSoftShadowMaskLowResOptimization,
        gbuffers,
        *renderGraph);
    
    // ============= Deferred lighting =============
    
    deferredLightingPass_->Render(
        *renderSceneCamera, gbuffers, skyLut, *renderGraph, framebuffer);

    // ============= ImGui =============

    imguiRenderer_->Render(
        frame.imguiDrawData.get(), framebuffer, renderGraph.get());

    // ============= Blit =============

    renderGraph->CreateBlitTexture2DPass("BlitToSwapchain", framebuffer, swapchainImage, true);

    // ============= Execution =============

    transientConstantBufferAllocator_->Flush();
    
    renderGraph->SetCompleteFence(device_->GetFrameFence());
    renderGraphExecuter_->Execute(renderGraph);
}

RTRC_RENDERER_END
