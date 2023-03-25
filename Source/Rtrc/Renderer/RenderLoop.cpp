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
    , tlasPool_        (device)
    , frameIndex_      (1)
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
    materialManager_.UpdateCachedMaterialData(frame);

    constexpr int MAX_BLAS_BUILD_COUNT = 5;
    constexpr int MAX_BLAS_BUILD_PRIMITIVE_COUNT = 1e5;
    auto buildBlasPass = meshManager_.BuildBlasForMeshes(
        *renderGraph, MAX_BLAS_BUILD_COUNT, MAX_BLAS_BUILD_PRIMITIVE_COUNT);
    
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

    Connect(buildBlasPass, imguiPass);
    Connect(clearPass, imguiPass);
    imguiPass->SetSignalFence(device_->GetFrameFence());
    renderGraphExecuter_->Execute(renderGraph);
}

RC<Tlas> RenderLoop::BuildOpaqueTlasForScene(
    const SceneProxy &scene, TransientScene &transientScene, CommandBuffer &commandBuffer)
{
    // Collect static mesh proxies with opaque blas flag enabled

    assert(transientScene.objectsInOpaqueTlas.empty());
    for(const StaticMeshRenderProxy *proxy : scene.GetStaticMeshRenderers())
    {
        if(!proxy->rayTracingFlags.contains(StaticMeshRendererRayTracingFlagBit::InOpaqueTlas))
        {
            continue;
        }
        auto cachedMeshData = meshManager_.FindCachedMesh(proxy->meshRenderingData->GetUniqueID());
        if(!cachedMeshData || !cachedMeshData->blas)
        {
            continue;
        }
        auto cachedMaterialData = materialManager_.FindCachedMaterial(proxy->materialRenderingData->GetUniqueID());
        if(!cachedMaterialData || !cachedMaterialData->albedoTextureEntry)
        {
            continue;
        }
        transientScene.objectsInOpaqueTlas.push_back({ proxy, cachedMeshData, cachedMaterialData });
    }

    // Prepare instance data

    std::vector<RHI::RayTracingInstanceData> opaqueTlasInstanceData;
    opaqueTlasInstanceData.reserve(transientScene.objectsInOpaqueTlas.size());

    std::vector<MaterialDataPerInstanceInOpaqueTlas> opaqueTlasMaterialData;
    opaqueTlasMaterialData.reserve(transientScene.objectsInOpaqueTlas.size());

    for(size_t i = 0; i < transientScene.objectsInOpaqueTlas.size(); ++i)
    {
        const auto [proxy, cachedMesh, cachedMaterial] = transientScene.objectsInOpaqueTlas[i];

        RHI::RayTracingInstanceData &instanceData = opaqueTlasInstanceData.emplace_back();
        std::memcpy(instanceData.transform3x4, &proxy->localToWorld[0][0], sizeof(instanceData.transform3x4));
        instanceData.instanceCustomIndex = cachedMesh->geometryBufferEntry.GetOffset();
        instanceData.instanceMask = 0xff;
        instanceData.instanceSbtOffset = 0;
        instanceData.flags = 0;
        instanceData.accelerationStructureAddress = cachedMesh->blas->GetRHIObject()->GetDeviceAddress().address;

        MaterialDataPerInstanceInOpaqueTlas &materialData = opaqueTlasMaterialData.emplace_back();
        materialData.albedoTextureIndex = cachedMaterial->albedoTextureEntry->GetOffset();
    }

    // Build tlas

    auto instanceDataBuffer = device_->CreateAndUploadBuffer(RHI::BufferDesc
        {
            .size = sizeof(RHI::RayTracingInstanceData) * opaqueTlasInstanceData.size(),
            .usage = RHI::BufferUsage::AccelerationStructureBuildInput | RHI::BufferUsage::DeviceAddress,
            .hostAccessType = RHI::BufferHostAccessType::SequentialWrite
        }, opaqueTlasInstanceData.data());

    const RHI::RayTracingInstanceArrayDesc instanceArrayDesc =
    {
        .instanceCount = static_cast<uint32_t>(opaqueTlasInstanceData.size()),
        .instanceData = instanceDataBuffer->GetRHIObject()->GetDeviceAddress(),
        .opaque = true,
    };

    auto prebuildInfo = device_->CreateTlasPrebuildInfo(
        instanceArrayDesc, RHI::RayTracingAccelerationStructureBuildFlagBit::PreferFastBuild);

    auto tlas = tlasPool_.GetTlas(prebuildInfo.GetAccelerationStructureBufferSize());
    // TODO

    // Upload material data

    transientScene.materialDataForOpaqueTlas = device_->CreateAndUploadBuffer(RHI::BufferDesc
        {
            .size = sizeof(MaterialDataPerInstanceInOpaqueTlas) * opaqueTlasMaterialData.size(),
            .usage = RHI::BufferUsage::ShaderStructuredBuffer | RHI::BufferUsage::TransferDst,
            .hostAccessType = RHI::BufferHostAccessType::None
        }, opaqueTlasMaterialData.data());
    transientScene.materialDataForOpaqueTlas->SetDefaultStructStride(sizeof(MaterialDataPerInstanceInOpaqueTlas));

    // TODO
    return {};
}

RTRC_RENDERER_END
