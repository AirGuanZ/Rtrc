#include <iostream>

#include <Rtrc/Rtrc.h>

using namespace Rtrc;

class Application : public Uncopyable
{
    Window window_;
    Input *input_ = nullptr;

    RHI::InstancePtr   instance_;
    Box<RenderContext> renderContext_;

    Box<RG::Executer> executer_;

    void Initialize()
    {
        window_ = WindowBuilder()
            .SetSize(800, 800)
            .SetTitle("Rtrc Sample: Deferred Shading")
            .Create();
        instance_ = CreateVulkanInstance(RHI::VulkanInstanceDesc
        {
            .extensions = Window::GetRequiredVulkanInstanceExtensions(),
            .debugMode = RTRC_DEBUG
        });
        renderContext_ = MakeBox<RenderContext>(instance_->CreateDevice(), &window_);
        executer_ = MakeBox<RG::Executer>(*renderContext_);
    }

    void Frame()
    {
        if(window_.GetInput().IsKeyDown(KeyCode::Escape))
        {
            window_.SetCloseFlag(true);
        }

        auto rg = renderContext_->CreateRenderGraph();
        auto renderTarget = rg->RegisterSwapchainTexture(renderContext_->GetSwapchain());
        auto clearPass = rg->CreatePass("Clear Render Target");
        clearPass->Use(renderTarget, RG::RENDER_TARGET_WRITE)
                 ->SetCallback([&](RG::PassContext &context)
                 {
                     auto rt = renderTarget->Get();
                     auto &cmd = context.GetCommandBuffer();
                     cmd.BeginRenderPass(ColorAttachment
                     {
                         .renderTargetView = renderTarget->Get()->GetRTV().rhiRTV,
                         .loadOp = AttachmentLoadOp::Clear,
                         .storeOp = AttachmentStoreOp::Store,
                         .clearValue = ColorClearValue{ 0, 1, 1, 0 }
                     });
                     cmd.EndRenderPass();
                 })
                 ->SetSignalFence(renderContext_->GetFrameFence());
        executer_->Execute(rg);
    }

public:

    void Run()
    {
        Initialize();
        renderContext_->BeginRenderLoop();
        while(!window_.ShouldClose())
        {
            if(!renderContext_->BeginFrame())
            {
                continue;
            }
            Frame();
            renderContext_->Present();
        }
        renderContext_->EndRenderLoop();
        renderContext_->WaitIdle();
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
