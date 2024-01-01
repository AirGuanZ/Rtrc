#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/ToolKit/Application/Application.h>

RTRC_BEGIN

namespace ApplicationDetail
{

    Application *appInst = nullptr;

} // namespace ApplicationDetail

Application &Application::GetInstance()
{
    assert(ApplicationDetail::appInst);
    return *ApplicationDetail::appInst;
}

Application::Application()
{
    assert(!ApplicationDetail::appInst);
    ApplicationDetail::appInst = this;
}

Application::~Application()
{
    assert(ApplicationDetail::appInst == this);
    ApplicationDetail::appInst = nullptr;
}

void Application::Run(const Config &config)
{
    window_ = WindowBuilder()
        .SetTitle(config.title)
        .SetSize(static_cast<int>(config.width), static_cast<int>(config.height))
        .SetMaximized(config.maximized)
        .Create();

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
        window_,
        config.backendType,
        RHI::Format::B8G8R8A8_UNorm,
        2,
        config.debug,
        config.vsync,
        deviceFlags);
    RTRC_SCOPE_EXIT{ device_->WaitIdle(); };

    window_.SetFocus();
    RTRC_SCOPE_EXIT{ window_.GetInput().LockCursor(false); };

    imgui_                  = MakeBox<ImGuiInstance>(device_, window_);
    resourceManager_        = MakeBox<ResourceManager>(device_);
    bindlessTextureManager_ = MakeBox<BindlessTextureManager>(device_);
    
    Initialize();
    UpdateLoop();
    Destroy();
}

bool Application::GetExitFlag() const
{
    return window_.ShouldClose();
}

void Application::SetExitFlag(bool shouldExit)
{
    window_.SetCloseFlag(shouldExit);
}

Window &Application::GetWindow()
{
    return window_;
}

WindowInput &Application::GetWindowInput()
{
    return window_.GetInput();
}

void Application::UpdateLoop()
{
    Vector2i framebufferSize = window_.GetFramebufferSize();
    frameTimer_.Restart();

    while(!window_.ShouldClose())
    {
        Window::DoEvents();

        // Handle window resize

        if(framebufferSize != window_.GetFramebufferSize() || framebufferSize.x == 0 || framebufferSize.y == 0)
        {
            framebufferSize = window_.GetFramebufferSize();
            ResizeFrameBuffer(framebufferSize.x, framebufferSize.y);
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
