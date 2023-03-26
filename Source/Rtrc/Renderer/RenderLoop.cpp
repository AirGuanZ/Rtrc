#include <Rtrc/Renderer/RenderLoop.h>
#include <Rtrc/Utility/Enumerate.h>
#include <Rtrc/Utility/Thread.h>

RTRC_RENDERER_BEGIN

RenderLoop::RenderLoop(
    ObserverPtr<Device>                       device,
    ObserverPtr<const BuiltinResourceManager> builtinResources,
    ObserverPtr<BindlessTextureManager>       bindlessTextures)
    : device_          (device)
    , builtinResources_(builtinResources)
    , bindlessTextures_(bindlessTextures)
    , meshManager_     (device)
    , frameIndex_      (1)
    , cachedScene_     (device)
{
    imguiRenderer_       = MakeBox<ImGuiRenderer>(device_);
    renderGraphExecuter_ = MakeBox<RG::Executer>(device_);
    renderThread_        = std::jthread(&RenderLoop::RenderThreadEntry, this);

    bindlessStructuredBuffersForBlas_ = MakeBox<BindlessBufferManager>(
        device_, 64, MAX_COUNT_BINDLESS_STRUCTURE_BUFFER_FOR_BLAS);
}

RenderLoop::~RenderLoop()
{
    assert(renderThread_.joinable());
    renderThread_.join();
}

void RenderLoop::AddCommand(RenderCommand command)
{
    renderCommandQueue_.push(std::move(command));
}

float RenderLoop::ComputeBuildBlasSortKey(const Vector3f &eye, const StaticMeshRenderProxy *renderer)
{
    if(renderer->rayTracingFlags == StaticMeshRendererRayTracingFlagBit::None)
    {
        return -1;
    }
    if(renderer->meshRenderingData->GetLayout()->GetVertexBufferLayouts()[0] != Mesh::BuiltinVertexBufferLayout::Default)
    {
        throw Exception(
            "Static mesh render object with ray tracing flag enabled must have a mesh "
            "using default builtin vertex buffer layout as the first vertex buffer layout");
    }
    const Vector3f nearestPointToEye = Max(renderer->worldBound.lower, Min(eye, renderer->worldBound.upper));
    const float distanceSquare = LengthSquare(nearestPointToEye - eye);
    const float radiusSquare = LengthSquare(renderer->worldBound.ComputeExtent());
    return radiusSquare / (std::max)(distanceSquare, 1e-5f);
}

const BindlessTextureEntry *RenderLoop::ExtractAlbedoTextureEntry(const MaterialInstance::SharedRenderingData *material)
{
    return material->GetPropertySheet().GetBindlessTextureEntry(RTRC_MATERIAL_PROPERTY_NAME(AlbedoTextureIndex));
}

void RenderLoop::RenderThreadEntry()
{
    SetThreadIndentifier(ThreadUtility::ThreadIdentifier::Render);

    bool isSwapchainInvalid = false;
    bool continueRenderLoop = true;

    device_->BeginRenderLoop();
    RTRC_SCOPE_EXIT{ device_->EndRenderLoop(); };

    frameTimer_.Restart();
    while(continueRenderLoop)
    {
        RenderCommand nextRenderCommand;
        if(renderCommandQueue_.try_pop(nextRenderCommand))
        {
            nextRenderCommand.Match(
                [&](const RenderCommand_ResizeFramebuffer &resize)
                {
                    RTRC_SCOPE_EXIT{ resize.finishSemaphore->release(); };
                    device_->GetRawDevice()->WaitIdle();
                    device_->RecreateSwapchain();
                    isSwapchainInvalid = false;
                },
                [&](const RenderCommand_RenderStandaloneFrame &frame)
                {
                    RTRC_SCOPE_EXIT
                    {
                        frame.finishSemaphore->release();
                        ++frameIndex_;
                    };
                    if(isSwapchainInvalid)
                    {
                        return;
                    }
                    if(!device_->BeginFrame(false))
                    {
                        isSwapchainInvalid = true;
                        return;
                    }
                    frameTimer_.BeginFrame();
                    RenderSingleFrame(frame);
                    if(!device_->Present())
                    {
                        isSwapchainInvalid = true;
                    }
                },
                [&](const RenderCommand_Exit &)
                {
                    continueRenderLoop = false;
                });
        }
    }
}

void RenderLoop::RenderSingleFrame(const RenderCommand_RenderStandaloneFrame &frame)
{
    auto renderGraph = device_->CreateRenderGraph();
    auto framebuffer = renderGraph->RegisterSwapchainTexture(device_->GetSwapchain());

    meshManager_.UpdateCachedMeshData(frame);
    constexpr int MAX_BLAS_BUILD_COUNT = 5;
    constexpr int MAX_BLAS_BUILD_PRIMITIVE_COUNT = 1e5;
    auto buildBlasPass = meshManager_.BuildBlasForMeshes(
        *renderGraph, MAX_BLAS_BUILD_COUNT, MAX_BLAS_BUILD_PRIMITIVE_COUNT);

    materialManager_.UpdateCachedMaterialData(frame);
    auto rgScene = cachedScene_.Update(frame, meshManager_, materialManager_, *renderGraph);

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

    auto imguiPass = imguiRenderer_->AddToRenderGraph(frame.imguiDrawData.get(), framebuffer, renderGraph.get());

    Connect(buildBlasPass, rgScene.buildTlasPass);
    Connect(clearPass, imguiPass);
    imguiPass->SetSignalFence(device_->GetFrameFence());
    renderGraphExecuter_->Execute(renderGraph);
}

RTRC_RENDERER_END
