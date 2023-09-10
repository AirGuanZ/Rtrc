#include <Rtrc/Application/Application.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>

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
        .SetSize(config.width, config.height)
        .SetMaximized(config.maximized)
        .Create();

    Device::Flags deviceFlags = Device::DisableAutoSwapchainRecreate;
    if(config.rayTracing)
    {
        deviceFlags |= Device::EnableRayTracing;
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

    activeScene_ = MakeBox<Scene>();
    
    const ApplicationInitializeContext initContext =
    {
        .device                 = device_.get(),
        .resourceManager        = resourceManager_.get(),
        .bindlessTextureManager = bindlessTextureManager_.get(),
        .activeScene            = activeScene_.get(),
        .activeCamera           = &activeCamera_
    };
    Initialize(initContext);

    activeRenderSettings_.enableRayTracing = config.rayTracing;
    renderLoop_ = MakeBox<Renderer::RenderLoop>(resourceManager_, bindlessTextureManager_);
    renderLoop_->BeginRenderLoop();

    RTRC_SCOPE_EXIT
    {
        renderLoop_->EndRenderLoop();
        renderLoop_.reset();
    };
    UpdateLoop();
}

void Application::Initialize(const ApplicationInitializeContext &context)
{
    // Do nothing
}

void Application::Update(const ApplicationUpdateContext &context)
{
    SetExitFlag(false);
    if(GetWindowInput().IsKeyDown(KeyCode::Escape))
    {
        SetExitFlag(true);
    }

    auto &imgui = *context.imgui;
    imgui.SetNextWindowSize({ 200, 200 }, ImGuiCond_Once);
    if(imgui.Begin("Rtrc Application"))
    {
        if(imgui.Button("Exit"))
        {
            SetExitFlag(true);
        }
    }
    imgui.End();
}

void Application::Destroy()
{
    // Do nothing
}

bool Application::GetExitFlag() const
{
    return window_.ShouldClose();
}

void Application::SetExitFlag(bool shouldExit)
{
    window_.SetCloseFlag(shouldExit);
}

BindlessTextureManager &Application::GetBindlessTextureManager()
{
    return *bindlessTextureManager_;
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
    Timer timer;
    while(!window_.ShouldClose())
    {
        Window::DoEvents();

        // Handle window resize

        if(framebufferSize != window_.GetFramebufferSize() || framebufferSize.x == 0 || framebufferSize.y == 0)
        {
            framebufferSize = window_.GetFramebufferSize();
            renderLoop_->ResizeFramebuffer(framebufferSize.x, framebufferSize.y);
        }

        // ImGui

        imgui_->BeginFrame();

        // Update

        timer.BeginFrame();
        const ApplicationUpdateContext updateContext =
        {
            .activeScene  = activeScene_.get(),
            .activeCamera = &activeCamera_,
            .imgui        = imgui_.get(),
            .frameTimer   = &timer,
        };
        Update(updateContext);

        // Send render command

        renderLoop_->SetRenderSettings(activeRenderSettings_);

        auto imguiDrawData = imgui_->Render();
        activeScene_->PrepareRender();

        Renderer::RenderLoop::FrameInput frameInput;
        frameInput.scene = activeScene_.get();
        frameInput.camera = &activeCamera_;
        frameInput.imguiDrawData = imguiDrawData.get();
        renderLoop_->RenderFrame(frameInput);
    }
}

RTRC_END
