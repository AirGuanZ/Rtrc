#include <iostream>

#include <Rtrc/Rtrc.h>

using namespace Rtrc;

class Application : public Uncopyable
{
    Window window_;
    Input *input_ = nullptr;
    
    Box<Device> device_;

    Box<RG::Executer> executer_;

    void Initialize()
    {
        window_ = WindowBuilder()
            .SetSize(800, 800)
            .SetTitle("Rtrc Sample: Deferred Shading")
            .Create();
        device_ = Device::CreateGraphicsDevice(window_);
        executer_ = MakeBox<RG::Executer>(device_.get());
    }

    void Frame()
    {
        if(window_.GetInput().IsKeyDown(KeyCode::Escape))
        {
            window_.SetCloseFlag(true);
        }

        auto rg = device_->CreateRenderGraph();
        auto renderTarget = rg->RegisterSwapchainTexture(device_->GetSwapchain());
        auto clearPass = rg->CreatePass("Clear Render Target");
        clearPass->Use(renderTarget, RG::RENDER_TARGET_WRITE)
                 ->SetCallback([&](RG::PassContext &context)
                 {
                     auto rt = renderTarget->Get();
                     auto &cmd = context.GetCommandBuffer();
                     cmd.BeginRenderPass(ColorAttachment
                     {
                         .renderTargetView = rt->CreateRTV().GetRHIObject(),
                         .loadOp = AttachmentLoadOp::Clear,
                         .storeOp = AttachmentStoreOp::Store,
                         .clearValue = ColorClearValue{ 0, 1, 1, 0 }
                     });
                     cmd.EndRenderPass();
                 })
                 ->SetSignalFence(device_->GetFrameFence());
        executer_->Execute(rg);
    }

public:

    void Run()
    {
        Initialize();
        device_->BeginRenderLoop();
        while(!window_.ShouldClose())
        {
            if(!device_->BeginFrame())
            {
                continue;
            }
            Frame();
            device_->Present();
        }
        device_->EndRenderLoop();
    }
};

int main()
{
    EnableMemoryLeakReporter();
    try
    {
        Application().Run();
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
