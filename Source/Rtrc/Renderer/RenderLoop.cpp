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

    bindlessStructuredBuffersForBlas_ = MakeBox<BindlessBufferManager>(
        device_, 64, MAX_COUNT_BINDLESS_STRUCTURE_BUFFER_FOR_BLAS);
    transientConstantBufferAllocator_ = MakeBox<TransientConstantBufferAllocator>(*device);

    renderThread_ = std::jthread(&RenderLoop::RenderThreadEntry, this);

    gbufferPass_          = MakeBox<GBufferPass>(device_);
    deferredLightingPass_ = MakeBox<DeferredLightingPass>(device_, builtinResources_);
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
                    RenderStandaloneFrame(frame);
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

void RenderLoop::RenderStandaloneFrame(const RenderCommand_RenderStandaloneFrame &frame)
{
    LinearAllocator linearAllocator;
    transientConstantBufferAllocator_->NewBatch();

    auto renderGraph = device_->CreateRenderGraph();
    auto framebuffer = renderGraph->RegisterSwapchainTexture(device_->GetSwapchain());

    // Update meshes

    meshManager_.UpdateCachedMeshData(frame);
    constexpr int MAX_BLAS_BUILD_COUNT = 5;
    constexpr int MAX_BLAS_BUILD_PRIMITIVE_COUNT = 1e5;
    auto buildBlasPass = meshManager_.BuildBlasForMeshes(
        *renderGraph, MAX_BLAS_BUILD_COUNT, MAX_BLAS_BUILD_PRIMITIVE_COUNT);

    // Update materials

    materialManager_.UpdateCachedMaterialData(frame);

    // Update scene

    auto rgScene = cachedScene_.Update(
        frame, *transientConstantBufferAllocator_,
        meshManager_, materialManager_, *renderGraph, linearAllocator);
    const CachedScenePerCamera *cachedScenePerCamera = cachedScene_.GetCachedScenePerCamera(frame.camera.originalId);
    assert(cachedScenePerCamera);

    // GBuffers

    const GBufferPass::RenderGraphOutput rgGBuffers = gbufferPass_->RenderGBuffers(
        *cachedScenePerCamera, *renderGraph, framebuffer->GetSize());

    // Clear frame buffer

    auto clearPass = renderGraph->CreatePass("Clear Framebuffer");
    clearPass->Use(framebuffer, RG::COLOR_ATTACHMENT_WRITEONLY);
    clearPass->SetCallback([&](RG::PassContext &context)
    {
        CommandBuffer &commandBuffer = context.GetCommandBuffer();
        commandBuffer.BeginRenderPass(ColorAttachment
        {
            .renderTargetView = framebuffer->Get(context)->CreateRtv(),
            .loadOp           = AttachmentLoadOp::Clear,
            .storeOp          = AttachmentStoreOp::Store,
            .clearValue       = ColorClearValue{ 0, 0, 0, 1 }
        });
        commandBuffer.EndRenderPass();
    });

    // Deferred lighting

    DeferredLightingPass::RenderGraphInput rgDeferredLightingInput;
    rgDeferredLightingInput.inGBuffers      = rgGBuffers.gbuffers;
    rgDeferredLightingInput.outRenderTarget = framebuffer;
    const DeferredLightingPass::RenderGraphOutput rgDeferredLighting = deferredLightingPass_->RenderDeferredLighting(
        *cachedScenePerCamera, *renderGraph, rgDeferredLightingInput);

    // Imgui

    auto imguiPass = imguiRenderer_->AddToRenderGraph(frame.imguiDrawData.get(), framebuffer, renderGraph.get());

    // Dependencies & execution

    Connect(buildBlasPass, rgScene.buildTlasPass);
    Connect(rgGBuffers.gbufferPass, rgDeferredLighting.lightingPass);
    Connect(clearPass, rgDeferredLighting.lightingPass);
    Connect(rgDeferredLighting.lightingPass, imguiPass);
    imguiPass->SetSignalFence(device_->GetFrameFence());

    transientConstantBufferAllocator_->Flush();
    renderGraphExecuter_->Execute(renderGraph);
}

RTRC_RENDERER_END
