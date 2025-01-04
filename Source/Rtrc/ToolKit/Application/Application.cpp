#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/ToolKit/Application/Application.h>

RTRC_BEGIN

Application::Application(int argc, char *argv[])
{
    programArguments_.reserve(argc);
    for(int i = 0; i < argc; ++i)
    {
        programArguments_.emplace_back(argv[i]);
    }
}

void Application::Run(const Config &config)
{
    window_ = MakeBox<Window>(WindowBuilder()
        .SetTitle(config.title)
        .SetSize(static_cast<int>(config.width), static_cast<int>(config.height))
        .SetMaximized(config.maximized)
        .Create());
    input_ = window_->GetInput();
    RTRC_SCOPE_EXIT{ input_ = {}; };

    Device::Flags deviceFlags = Device::DisableAutoSwapchainRecreate;
    if(config.rayTracing)
    {
        deviceFlags |= Device::EnableRayTracing;
    }

    if(config.enableGPUCapturer)
    {
#if RTRC_PIX_CAPTURER
        gpuCapturer_ = RHI::CreatePIXCapturer();
#endif
    }

    device_ = Device::CreateGraphicsDevice(
    {
        .window              = window_,
        .rhiType             = config.backendType,
        .swapchainFormat     = RHI::Format::B8G8R8A8_UNorm,
        .swapchainImageCount = 3,
        .debugMode           = config.debug,
        .vsync               = config.vsync,
        .flags               = deviceFlags
    });
    RTRC_SCOPE_EXIT{ device_->WaitIdle(); };

    window_->SetFocus();
    RTRC_SCOPE_EXIT{ input_->LockCursor(false); };

    imgui_ = MakeBox<ImGuiInstance>(device_, window_);
    resourceManager_ = MakeBox<ResourceManager>(device_);
    bindlessTextureManager_ = MakeBox<BindlessTextureManager>(device_);
    
    Initialize();
    UpdateLoop();
    Destroy();
}

bool Application::GetExitFlag() const
{
    return window_->ShouldClose();
}

void Application::SetExitFlag(bool shouldExit)
{
    window_->SetCloseFlag(shouldExit);
}

Window &Application::GetWindow()
{
    return *window_;
}

WindowInput &Application::GetWindowInput()
{
    return *input_;
}

void Application::UpdateLoop()
{
    Vector2i framebufferSize = window_->GetFramebufferSize();
    frameTimer_.Restart();

    while(!window_->ShouldClose())
    {
        Window::DoEvents();

        // Handle window resize

        if(framebufferSize != window_->GetFramebufferSize() || framebufferSize.x == 0 || framebufferSize.y == 0)
        {
            framebufferSize = window_->GetFramebufferSize();
            ResizeFrameBuffer(framebufferSize.x, framebufferSize.y);
        }

        if(!isCapturing_ && !pendingGPUCaptureFrames_)
        {
            auto &input = window_->GetInput();
            if(input.IsKeyPressed(KeyCode::LeftShift) && input.IsKeyDown(KeyCode::F11))
            {
                pendingGPUCaptureFrames_ = 1;
            }
        }

        // Begin capture

        if(!isCapturing_ && pendingGPUCaptureFrames_)
        {
            device_->WaitIdle();
            isCapturing_ = true;
            if(gpuCapturer_)
            {
                if(!gpuCapturer_->Begin(gpuCaptureOutputPrefix_))
                {
                    LogWarning("Fail to begin gpu capture");
                    isCapturing_ = false;
                    pendingGPUCaptureFrames_ = 0;
                }
            }
        }

        // ImGui

        imgui_->BeginFrame();

        // Update

        frameTimer_.BeginFrame();
        Update();

        // End capture

        if(isCapturing_ && !--pendingGPUCaptureFrames_)
        {
            device_->WaitIdle();
            isCapturing_ = false;
            if(gpuCapturer_ && !gpuCapturer_->End())
            {
                LogWarning("Fail to end gpu capture");
            }
        }
    }
}

RTRC_END
