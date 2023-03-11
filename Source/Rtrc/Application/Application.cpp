#include <Rtrc/Application/Application.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>

RTRC_BEGIN

void Application::Run(const Config &config)
{
    window_ = WindowBuilder()
        .SetTitle(config.title)
        .SetSize(config.width, config.height)
        .Create();

    device_ = Device::CreateGraphicsDevice(
        window_, RHI::Format::B8G8R8A8_UNorm, 2, config.debug,
        config.vsync, Device::EnableRayTracing | Device::EnableSwapchainUav | Device::DisableAutoSwapchainRecreate);
    RTRC_SCOPE_EXIT{ device_->WaitIdle(); };

    window_.SetFocus();
    RTRC_SCOPE_EXIT{ window_.GetInput().LockCursor(false); };

    renderGraphExecutor_ = MakeBox<RG::Executer>(device_.get());

    renderThread_ = std::jthread(&Application::RenderLoop, this);
    RTRC_SCOPE_EXIT
    {
        assert(renderThread_.joinable());
        renderThread_.join();
    };

    try
    {
        UpdateLoop();
    }
    catch(...)
    {
        throw;
    }
}

void Application::UpdateLoop()
{
    RTRC_SCOPE_EXIT{ renderCommandQueue_.push(ExitRenderLoop{}); };

    std::binary_semaphore framebufferResizeSemaphore(0);
    std::binary_semaphore finishRenderSemaphore(1);

    Vector2i framebufferSize = window_.GetFramebufferSize();
    while(!window_.ShouldClose())
    {
        Window::DoEvents();
        if(window_.GetInput().IsKeyDown(KeyCode::Escape))
        {
            window_.SetCloseFlag(true);
        }

        if(framebufferSize != window_.GetFramebufferSize())
        {
            framebufferSize = window_.GetFramebufferSize();
            renderCommandQueue_.push(ResizeFramebuffer
            {
                .width     = static_cast<uint32_t>(framebufferSize.x),
                .height    = static_cast<uint32_t>(framebufferSize.y),
                .semaphore = &framebufferResizeSemaphore
            });
            framebufferResizeSemaphore.acquire();
        }

        finishRenderSemaphore.acquire();
        renderCommandQueue_.push(TryRenderFrame{ {}, {}, &finishRenderSemaphore });
    }
}

void Application::RenderLoop()
{
    device_->BeginRenderLoop();
    RTRC_SCOPE_EXIT{ device_->EndRenderLoop(); };
    bool needRecreatingSwapchain = false;

    frameTimer_.Restart();
    while(true)
    {
        RenderCommand nextRenderCommand;
        if(renderCommandQueue_.try_pop(nextRenderCommand))
        {
            if(nextRenderCommand.Is<ExitRenderLoop>())
            {
                return;
            }

            nextRenderCommand.Match(
                [&](const ResizeFramebuffer &resize)
                {
                    device_->GetRawDevice()->WaitIdle();
                    device_->RecreateSwapchain();
                    resize.semaphore->release();
                    needRecreatingSwapchain = false;
                },
                [&](const TryRenderFrame &tryRenderFrame)
                {
                    RTRC_SCOPE_EXIT{ tryRenderFrame.finishRenderSemaphore->release(); };
                    if(needRecreatingSwapchain)
                    {
                        return;
                    }
                    if(!device_->BeginFrame(false))
                    {
                        needRecreatingSwapchain = true;
                        return;
                    }
                    frameTimer_.BeginFrame();
                    Render(&tryRenderFrame.cameraProxy, tryRenderFrame.sceneProxy.get());
                    if(!device_->Present())
                    {
                        needRecreatingSwapchain = true;
                    }
                },
                [](const ExitRenderLoop &) { });
        }
    }
}

void Application::Render(const Camera *camera, const SceneProxy *scene)
{
    auto renderGraph = device_->CreateRenderGraph();
    auto framebuffer = renderGraph->RegisterSwapchainTexture(device_->GetSwapchain());

    auto clearPass = renderGraph->CreatePass("Clear Framebuffer");
    clearPass->Use(framebuffer, RG::COLOR_ATTACHMENT_WRITEONLY);
    clearPass->SetCallback([&](RG::PassContext &context)
    {
        CommandBuffer &commandBuffer = context.GetCommandBuffer();
        commandBuffer.BeginRenderPass(ColorAttachment
        {
            .renderTargetView = framebuffer->Get()->CreateRtv(),
            .loadOp           = AttachmentLoadOp::Clear,
            .storeOp          = AttachmentStoreOp::Store,
            .clearValue       = ColorClearValue{ 0, 1, 1, 1 }
        });
        commandBuffer.EndRenderPass();
    });

    clearPass->SetSignalFence(device_->GetFrameFence());
    renderGraphExecutor_->Execute(renderGraph);
}

RTRC_END
