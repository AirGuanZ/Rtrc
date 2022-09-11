#include <iostream>

#include <Rtrc/Rtrc.h>

using namespace Rtrc;

class Application : public Uncopyable
{
    Window window_;
    Input *input_ = nullptr;

    RHI::InstancePtr  instance_;
    RHI::DevicePtr    device_;
    RHI::QueuePtr     graphicsQueue_;
    RHI::SwapchainPtr swapchain_;

    RC<FrameResourceManager> frameResources_;
    Box<RG::Executer> executer_;

    void InitializeVulkan()
    {
        window_ = WindowBuilder()
            .SetSize(800, 600)
            .SetTitle("Rtrc Sample: DeferredShading")
            .Create();
        input_ = &window_.GetInput();

        instance_ = CreateVulkanInstance(RHI::VulkanInstanceDesc
            {
                .extensions = Window::GetRequiredVulkanInstanceExtensions(),
                .debugMode = RTRC_DEBUG
            });
        device_ = instance_->CreateDevice();
        graphicsQueue_ = device_->GetQueue(RHI::QueueType::Graphics);

        RecreateSwapchain();
        window_.Attach([&](const WindowResizeEvent &e)
        {
            if(e.width > 0 && e.height > 0)
            {
                RecreateSwapchain();
            }
        });
    }

    void RecreateSwapchain()
    {
        device_->WaitIdle();
        swapchain_.Reset();
        swapchain_ = device_->CreateSwapchain(RHI::SwapchainDesc
        {
            .format = RHI::Format::B8G8R8A8_UNorm,
            .imageCount = 3
        }, window_);
    }

public:

    Application()
    {
        InitializeVulkan();

        frameResources_ = MakeRC<FrameResourceManager>(device_, swapchain_->GetRenderTargetCount());
        executer_ = MakeBox<RG::Executer>(frameResources_.get());
    }

    ~Application()
    {
        if(device_)
        {
            device_->WaitIdle();
        }
    }

    bool Frame()
    {
        // process window events

        Window::DoEvents();
        if(input_->IsKeyDown(KeyCode::Escape))
        {
            window_.SetCloseFlag(true);
        }
        if(!window_.HasFocus() || window_.GetFramebufferSize().x <= 0 || window_.GetFramebufferSize().y <= 0)
        {
            return true;
        }

        // begin frame

        frameResources_->BeginFrame();
        if(!swapchain_->Acquire())
        {
            return true;
        }

        // render graph

        RG::RenderGraph graph;
        graph.SetQueue(graphicsQueue_);

        auto renderTarget = graph.RegisterSwapchainTexture(swapchain_);

        auto clearPass = graph.CreatePass("Clear Render Target");
        clearPass
            ->Use(renderTarget, RG::RENDER_TARGET_WRITE)
            ->SetCallback([&](RG::PassContext &context)
            {
                auto rt = renderTarget->GetRHI();
                auto commandBuffer = context.GetRHICommandBuffer();
                commandBuffer->BeginRenderPass(RHI::RenderPassColorAttachment
                {
                    .renderTargetView = rt->CreateRTV(),
                    .loadOp           = RHI::AttachmentLoadOp::Clear,
                    .storeOp          = RHI::AttachmentStoreOp::Store,
                    .clearValue       = RHI::ColorClearValue{ 0, 1, 1, 0 }
                });
                commandBuffer->EndRenderPass();
            })
            ->SetSignalFence(frameResources_->GetFrameFence());

        executer_->Execute(graph);
        swapchain_->Present();

        return !window_.ShouldClose();
    }
};

int main()
{
    EnableMemoryLeakReporter();
    try
    {
        Application app;
        while(app.Frame())
        {
            
        }
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
