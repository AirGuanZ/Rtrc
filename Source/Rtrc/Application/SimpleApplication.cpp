
#include <Graphics/RenderGraph/Executable.h>
#include <Rtrc/Application/SimpleApplication.h>

RTRC_BEGIN

void SimpleApplication::ExecuteStandaloneRenderGraph(
    ObserverPtr<RG::RenderGraph> graph, bool enableTransientResourcePool)
{
    renderGraphExecuter_->Execute(graph, enableTransientResourcePool);
}

void SimpleApplication::Initialize()
{
    auto device = GetDevice();
    renderGraphExecuter_ = MakeBox<RG::Executer>(device);
    imguiRenderer_ = MakeBox<ImGuiRenderer>(device);
    isSwapchainInvalid = false;
    InitializeSimpleApplication();
    device->BeginRenderLoop();
}

void SimpleApplication::Update()
{
    if(isSwapchainInvalid)
    {
        return;
    }

    if(!GetDevice()->BeginFrame(false))
    {
        isSwapchainInvalid = true;
        return;
    }

    renderGraphExecuter_->NewFrame();

    auto graph = GetDevice()->CreateRenderGraph();
    UpdateSimpleApplication(graph);

    auto renderTarget = graph->RegisterSwapchainTexture(GetDevice()->GetSwapchain());
    auto imguiDrawData = GetImGuiInstance()->Render();
    imguiRenderer_->Render(imguiDrawData.get(), renderTarget, graph.get());

    renderGraphExecuter_->Execute(graph);

    if(!GetDevice()->Present())
    {
        isSwapchainInvalid = true;
    }
}

void SimpleApplication::Destroy()
{
    GetDevice()->EndRenderLoop();
    DestroySimpleApplication();
}

void SimpleApplication::ResizeFrameBuffer(uint32_t width, uint32_t height)
{
    if(width > 0 && height > 0)
    {
        GetDevice()->WaitIdle();
        GetDevice()->RecreateSwapchain();
        isSwapchainInvalid = false;
    }
}

RTRC_END