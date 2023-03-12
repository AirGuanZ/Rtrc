#include <Rtrc/Application/Application.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>

RTRC_BEGIN

void Application::Run(const Config &config)
{
    window_ = WindowBuilder()
        .SetTitle(config.title)
        .SetSize(config.width, config.height)
        .SetMaximized(config.maximized)
        .Create();

    device_ = Device::CreateGraphicsDevice(
        window_,
        RHI::Format::B8G8R8A8_UNorm,
        2,
        config.debug,
        config.vsync,
        Device::EnableRayTracing | Device::EnableSwapchainUav | Device::DisableAutoSwapchainRecreate);
    RTRC_SCOPE_EXIT{ device_->WaitIdle(); };

    window_.SetFocus();
    RTRC_SCOPE_EXIT{ window_.GetInput().LockCursor(false); };

    imgui_ = MakeBox<ImGuiInstance>(*device_, window_);

    renderLoop_ = MakeBox<RenderLoop>(*device_);
    
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
    RTRC_SCOPE_EXIT{ renderLoop_->AddCommand(RenderLoop::Command_Exit{}); };

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
            renderLoop_->AddCommand(RenderLoop::Command_ResizeFramebuffer
            {
                .width           = static_cast<uint32_t>(framebufferSize.x),
                .height          = static_cast<uint32_t>(framebufferSize.y),
                .finishSemaphore = &framebufferResizeSemaphore
            });
            framebufferResizeSemaphore.acquire();
        }

        imgui_->BeginFrame();

        if(imgui_->Begin("Test", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if(imgui_->Button("Exit"))
            {
                window_.SetCloseFlag(true);
            }
        }
        imgui_->End();

        RenderLoop::Command_RenderFrame renderFrame;
        renderFrame.cameraProxy = {};
        renderFrame.sceneProxy = {};
        renderFrame.imguiDrawData = imgui_->Render();
        renderFrame.finishSemaphore = &finishRenderSemaphore;
        finishRenderSemaphore.acquire();
        renderLoop_->AddCommand(std::move(renderFrame));
    }
}

RTRC_END
