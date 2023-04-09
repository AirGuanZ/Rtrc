#include <Rtrc/Application/Application.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/Utility/Thread.h>

RTRC_BEGIN

void Application::Run(const Config &config)
{
    SetThreadMode(ThreadUtility::ThreadMode::Standard);
    SetThreadIndentifier(ThreadUtility::ThreadIdentifier::Main);

    window_ = WindowBuilder()
        .SetTitle(config.title)
        .SetSize(config.width, config.height)
        .SetMaximized(config.maximized)
        .Create();

    Device::Flags deviceFlags = Device::EnableSwapchainUav | Device::DisableAutoSwapchainRecreate;
    if(config.rayTracing)
    {
        deviceFlags |= Device::EnableRayTracing;
    }

    device_ = Device::CreateGraphicsDevice(
        window_,
        RHI::Format::B8G8R8A8_UNorm,
        2,
        config.debug,
        config.vsync,
        deviceFlags);
    RTRC_SCOPE_EXIT{ device_->WaitIdle(); };

    window_.SetFocus();
    RTRC_SCOPE_EXIT{ window_.GetInput().LockCursor(false); };

    imgui_                 = MakeBox<ImGuiInstance>(device_, window_);
    resourceManager_       = MakeBox<ResourceManager>(device_);
    bindlessTextureManager = MakeBox<BindlessTextureManager>(device_);

    activeScene_ = MakeBox<Scene>();
    
    const ApplicationInitializeContext initContext =
    {
        .device                 = device_.get(),
        .resourceManager        = resourceManager_.get(),
        .bindlessTextureManager = bindlessTextureManager.get(),
        .activeScene            = activeScene_.get(),
        .activeCamera           = &activeCamera_
    };
    Initialize(initContext);

    RenderLoop::Config renderLoopConfig;
    renderLoopConfig.rayTracing = config.rayTracing;
    renderLoopConfig.handleCrossThreadException = config.handleCrossThreadException;
    renderLoop_ = MakeBox<RenderLoop>(
        renderLoopConfig, device_, resourceManager_->GetBuiltinResources(), bindlessTextureManager);

    RTRC_SCOPE_EXIT{ renderLoop_->AddCommand(Renderer::RenderCommand_Exit{}); };
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
    std::binary_semaphore framebufferResizeSemaphore(0);

    std::binary_semaphore finishRenderSemaphore(1);
    bool shouldWaitFinishRenderSemaphore = false;

    auto BeforeAddNextCommand = [&]
    {
        if(shouldWaitFinishRenderSemaphore)
        {
            finishRenderSemaphore.acquire();
            shouldWaitFinishRenderSemaphore = false;
        }
        if(renderLoop_->HasException())
        {
            std::rethrow_exception(renderLoop_->GetExceptionPointer());
        }
    };

    Vector2i framebufferSize = window_.GetFramebufferSize();
    Timer timer;
    while(!window_.ShouldClose())
    {
        Window::DoEvents();

        // Handle window resize

        if(framebufferSize != window_.GetFramebufferSize())
        {
            framebufferSize = window_.GetFramebufferSize();
            BeforeAddNextCommand();
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

        activeScene_->PrepareRendering();
        auto activeSceneProxy = activeScene_->CreateSceneProxy();

        Renderer::RenderCommand_RenderStandaloneFrame frame;
        frame.scene           = std::move(activeSceneProxy);
        frame.camera          = activeCamera_.GetRenderCamera();
        frame.imguiDrawData   = imgui_->Render();
        frame.finishSemaphore = &finishRenderSemaphore;

        BeforeAddNextCommand();
        renderLoop_->AddCommand(std::move(frame));
        shouldWaitFinishRenderSemaphore = true;
    }
}

RTRC_END
