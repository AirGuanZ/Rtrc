#include <Rtrc/Renderer/RenderLoop.h>

RTRC_BEGIN

RenderLoop::RenderLoop(Device &device)
    : device_(device)
{
    imguiRenderer_ = MakeBox<ImGuiRenderer>(device_);
    renderGraphExecuter_ = MakeBox<RG::Executer>(device_);
    renderThread_ = std::jthread(&RenderLoop::RenderThreadEntry, this);
}

RenderLoop::~RenderLoop()
{
    assert(renderThread_.joinable());
    renderThread_.join();
}

void RenderLoop::AddCommand(Command command)
{
    renderCommandQueue_.push(std::move(command));
}

void RenderLoop::RenderThreadEntry()
{
    bool isSwapchainInvalid = false;
    bool continueRenderLoop = true;

    device_.BeginRenderLoop();
    RTRC_SCOPE_EXIT{ device_.EndRenderLoop(); };

    frameTimer_.Restart();
    while(continueRenderLoop)
    {
        Command nextRenderCommand;
        if(renderCommandQueue_.try_pop(nextRenderCommand))
        {
            nextRenderCommand.Match(
                [&](const Command_ResizeFramebuffer &resize)
                {
                    RTRC_SCOPE_EXIT{ resize.finishSemaphore->release(); };
                    device_.GetRawDevice()->WaitIdle();
                    device_.RecreateSwapchain();
                    isSwapchainInvalid = false;
                },
                [&](const Command_RenderFrame &renderFrame)
                {
                    RTRC_SCOPE_EXIT{ renderFrame.finishSemaphore->release(); };
                    if(isSwapchainInvalid)
                    {
                        return;
                    }
                    if(!device_.BeginFrame(false))
                    {
                        isSwapchainInvalid = true;
                        return;
                    }
                    frameTimer_.BeginFrame();
                    RenderSingleFrame(renderFrame);
                    if(!device_.Present())
                    {
                        isSwapchainInvalid = true;
                    }
                },
                [&](const Command_Exit &)
                {
                    continueRenderLoop = false;
                });
        }
    }
}

void RenderLoop::RenderSingleFrame(const Command_RenderFrame &frameData)
{
    auto renderGraph = device_.CreateRenderGraph();
    auto framebuffer = renderGraph->RegisterSwapchainTexture(device_.GetSwapchain());

    auto clearPass = renderGraph->CreatePass("Clear Framebuffer");
    clearPass->Use(framebuffer, RG::COLOR_ATTACHMENT_WRITEONLY);
    clearPass->SetCallback([&](RG::PassContext &context)
    {
        CommandBuffer &commandBuffer = context.GetCommandBuffer();
        commandBuffer.BeginRenderPass(ColorAttachment
        {
            .renderTargetView = framebuffer->Get()->CreateRtv(),
            .loadOp           = AttachmentLoadOp::Clear,
            .storeOp          = AttachmentStoreOp::Store,
            .clearValue       = ColorClearValue{ 0, 1, 1, 1 }
        });
        commandBuffer.EndRenderPass();
    });

    auto imguiPass = imguiRenderer_->AddToRenderGraph(frameData.imguiDrawData.get(), framebuffer, renderGraph.get());

    Connect(clearPass, imguiPass);
    imguiPass->SetSignalFence(device_.GetFrameFence());
    renderGraphExecuter_->Execute(renderGraph);
}

RTRC_END
