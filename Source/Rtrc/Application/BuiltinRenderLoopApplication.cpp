#include <Rtrc/Application/BuiltinRenderLoopApplication.h>

RTRC_BEGIN

void BuiltinRenderLoopApplication::Initialize()
{
    activeScene_ = MakeBox<Scene>();
    renderLoop_ = MakeBox<Renderer::RealTimeRenderLoop>(GetResourceManager(), GetBindlessTextureManager());
    renderLoop_->BeginRenderLoop();
    InitializeLogic();
}

void BuiltinRenderLoopApplication::Update()
{
    UpdateLogic();

    auto imguiDrawData = GetImGuiInstance()->Render();
    renderLoop_->SetRenderSettings(activeRenderSettings_);
    activeScene_->PrepareRender();

    Renderer::RealTimeRenderLoop::FrameInput frameInput;
    frameInput.scene = activeScene_.get();
    frameInput.camera = &activeCamera_;
    frameInput.imguiDrawData = imguiDrawData.get();
    renderLoop_->RenderFrame(frameInput);
}

void BuiltinRenderLoopApplication::Destroy()
{
    renderLoop_->EndRenderLoop();
    DestroyLogic();
}

void BuiltinRenderLoopApplication::ResizeFrameBuffer(uint32_t width, uint32_t height)
{
    renderLoop_->ResizeFramebuffer(width, height);
}

RTRC_END
