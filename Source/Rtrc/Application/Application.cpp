#include <Rtrc/Application/Application.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/Utility/Thread.h>

RTRC_BEGIN

void Application::Run(const Config &config)
{
    SetThreadMode(ThreadUtility::ThreadMode::Standard);

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

    imgui_                 = MakeBox<ImGuiInstance>(device_, window_);
    resourceManager_       = MakeBox<ResourceManager>(device_);
    bindlessTextureManager = MakeBox<BindlessTextureManager>(device_);

    activeScene_ = MakeBox<Scene>();

    renderLoop_ = MakeBox<RenderLoop>(device_, resourceManager_->GetBuiltinResources(), bindlessTextureManager);
    RTRC_SCOPE_EXIT{ renderLoop_->AddCommand(Renderer::RenderCommand_Exit{}); };

    UpdateLoop();
}

void Application::UpdateLoop()
{
    SetThreadIndentifier(ThreadUtility::ThreadIdentifier::Main);

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

        // Handle window resize

        if(framebufferSize != window_.GetFramebufferSize())
        {
            framebufferSize = window_.GetFramebufferSize();
            renderLoop_->AddCommand(Renderer::RenderCommand_ResizeFramebuffer
            {
                .width           = static_cast<uint32_t>(framebufferSize.x),
                .height          = static_cast<uint32_t>(framebufferSize.y),
                .finishSemaphore = &framebufferResizeSemaphore
            });
            framebufferResizeSemaphore.acquire();
        }

        // ImGui

        imgui_->BeginFrame();

        if(imgui_->Begin("Test"))
        {
            if(imgui_->Button("Exit"))
            {
                window_.SetCloseFlag(true);
            }
        }
        imgui_->End();

        // Send render command

        {
            activeScene_->PrepareRendering();
            auto activeSceneProxy = activeScene_->CreateSceneProxy();

            Renderer::RenderCommand_RenderStandaloneFrame frame;
            frame.scene = std::move(activeSceneProxy);
            frame.camera = activeCamera_;
            frame.imguiDrawData = imgui_->Render();
            frame.finishSemaphore = &finishRenderSemaphore;
            finishRenderSemaphore.acquire();
            renderLoop_->AddCommand(std::move(frame));
        }
    }
}

RTRC_END
