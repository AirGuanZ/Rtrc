#include <iostream>

#include <Rtrc/Rtrc.h>

using namespace Rtrc;

void Run()
{
    // window & device

    auto window = WindowBuilder()
        .SetSize(800, 800)
        .SetTitle("Rtrc Sample TexturedQuad")
        .Create();

    auto &input = window.GetInput();

    auto instance = CreateVulkanInstance(RHI::VulkanInstanceDesc
        {
            .extensions = Window::GetRequiredVulkanInstanceExtensions(),
            .debugMode = true
        });

    auto device = instance->CreateDevice();

    auto graphicsQueue = device->GetQueue(RHI::QueueType::Graphics);

    // swapchain

    RHI::Ptr<RHI::Swapchain> swapchain;

    auto createSwapchain = [&]
    {
        device->WaitIdle();
        swapchain.Reset();
        swapchain = device->CreateSwapchain(RHI::SwapchainDesc
        {
            .format = RHI::Format::B8G8R8A8_UNorm,
            .imageCount = 3
        }, window);
    };

    createSwapchain();

    window.Attach([&](const WindowResizeEvent &e)
    {
        if(e.width > 0 && e.height > 0)
        {
            createSwapchain();
        }
    });

    // frame resources

    FrameResourceManager frameResources(device, swapchain->GetRenderTargetCount());

    // pipeline

    ShaderManager shaderManager(device);
    shaderManager.SetFileLoader("Asset/01.TexturedQuad/");

    std::string preprocessedSource;
    auto shader = shaderManager.AddShader({
        .VS = { .filename = "Quad.hlsl", .entry = "VSMain", .dumpedPreprocessedSource = &preprocessedSource },
        .FS = { .filename = "Quad.hlsl", .entry = "FSMain" }
    });
    std::cout << preprocessedSource << std::endl;

    auto bindingGroupLayout = shaderManager.GetBindingGroupLayoutByName("TestGroup");
    auto bindingLayout = shader->GetRHIBindingLayout();

    auto pipelineBuilder = device->CreateGraphicsPipelineBuilder();
    auto pipeline = (*pipelineBuilder)
        .SetVertexShader(shader->GetRawShader(RHI::ShaderStage::VertexShader))
        .SetFragmentShader(shader->GetRawShader(RHI::ShaderStage::FragmentShader))
        .AddColorAttachment(swapchain->GetRenderTargetDesc().format)
        .SetBindingLayout(bindingLayout)
        .SetViewports(1)
        .SetScissors(1)
        .CreatePipeline();

    // uploader

    ResourceUploader uploader(device);
    std::vector<RHI::BufferAcquireBarrier> uploadBufferAcquireBarriers;
    std::vector<RHI::TextureAcquireBarrier> uploadTextureAcquireBarriers;

    // vertex position buffer

    const std::array vertexPositionData =
    {
        Vector2f(-0.8f, -0.8f),
        Vector2f(-0.8f, +0.8f),
        Vector2f(+0.8f, +0.8f),
        Vector2f(-0.8f, -0.8f),
        Vector2f(+0.8f, +0.8f),
        Vector2f(+0.8f, -0.8f)
    };

    auto vertexPositionBuffer = device->CreateBuffer(RHI::BufferDesc
    {
        .size                 = sizeof(vertexPositionData),
        .usage                = RHI::BufferUsage::ShaderBuffer,
        .hostAccessType       = RHI::BufferHostAccessType::None,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });

    auto vertexPositionBufferSRV = vertexPositionBuffer->CreateSRV(RHI::BufferSRVDesc{
        .format = RHI::Format::R32G32_Float,
        .offset = 0,
        .range  = sizeof(vertexPositionData)
    });

    uploadBufferAcquireBarriers.push_back(uploader.Upload(
        vertexPositionBuffer, 0, sizeof(vertexPositionData), vertexPositionData.data(),
        graphicsQueue, RHI::PipelineStage::VertexShader, RHI::ResourceAccess::BufferRead));

    // vertex texcoord buffer

    const std::array vertexTexCoordData = {
        Vector2f(0.0f, 1.0f),
        Vector2f(0.0f, 0.0f),
        Vector2f(1.0f, 0.0f),
        Vector2f(0.0f, 1.0f),
        Vector2f(1.0f, 0.0f),
        Vector2f(1.0f, 1.0f)
    };

    auto vertexTexCoordBuffer = device->CreateBuffer(RHI::BufferDesc{
        .size                 = sizeof(vertexTexCoordData),
        .usage                = RHI::BufferUsage::ShaderBuffer,
        .hostAccessType       = RHI::BufferHostAccessType::None,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });

    auto vertexTexCoordBufferSRV = vertexTexCoordBuffer->CreateSRV(RHI::BufferSRVDesc{
        .format = RHI::Format::R32G32_Float,
        .offset = 0,
        .range  = sizeof(vertexTexCoordData)
    });

    uploadBufferAcquireBarriers.push_back(uploader.Upload(
        vertexTexCoordBuffer, 0, sizeof(vertexTexCoordData), vertexTexCoordData.data(),
        graphicsQueue, RHI::PipelineStage::VertexShader, RHI::ResourceAccess::BufferRead));

    // main texture

    auto mainTexData = ImageDynamic::Load("Asset/01.TexturedQuad/MainTexture.png");

    auto mainTex = device->CreateTexture2D(RHI::Texture2DDesc{
        .format               = RHI::Format::B8G8R8A8_UNorm,
        .width                = mainTexData.GetWidth(),
        .height               = mainTexData.GetHeight(),
        .mipLevels            = 1,
        .arraySize            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::TransferDst,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });

    mainTex->SetName("MainTexture");

    auto mainTexSRV = mainTex->Create2DSRV(RHI::Texture2DSRVDesc{
        .format         = RHI::Format::Unknown,
        .baseMipLevel   = 0,
        .levelCount     = 1,
        .baseArrayLayer = 0,
        .layerCount     = 1
    });

    uploadTextureAcquireBarriers.push_back(uploader.Upload(
        mainTex, 0, 0, mainTexData,
        graphicsQueue,
        RHI::PipelineStage::FragmentShader,
        RHI::ResourceAccess::TextureRead,
        RHI::TextureLayout::ShaderTexture));

    // main sampler

    auto mainSampler = device->CreateSampler(RHI::SamplerDesc{
        .magFilter = RHI::FilterMode::Linear,
        .minFilter = RHI::FilterMode::Linear,
        .mipFilter = RHI::FilterMode::Point,
        .addressModeU = RHI::AddressMode::Clamp,
        .addressModeV = RHI::AddressMode::Clamp,
        .addressModeW = RHI::AddressMode::Clamp
    });

    // upload

    uploader.SubmitAndSync();

    // binding group

    auto bindingGroup = bindingGroupLayout->CreateBindingGroup();
    bindingGroup->Set("VertexPositionBuffer", vertexPositionBufferSRV);
    bindingGroup->Set("VertexTexCoordBuffer", vertexTexCoordBufferSRV);
    bindingGroup->Set("MainTexture", mainTexSRV);
    bindingGroup->Set("MainSampler", mainSampler);

    // render loop

    while(!window.ShouldClose())
    {
        Window::DoEvents();

        if(input.IsKeyDown(KeyCode::Escape))
        {
            window.SetCloseFlag(true);
        }

        if(!window.HasFocus() || window.GetFramebufferSize().x <= 0 || window.GetFramebufferSize().y <= 0)
        {
            continue;
        }

        frameResources.BeginFrame();

        if(!swapchain->Acquire())
        {
            continue;
        }
        
        auto commandBuffer = frameResources.AllocateCommandBuffer(RHI::QueueType::Graphics);

        auto image = swapchain->GetRenderTarget();
        auto rtv = image->Create2DRTV(RHI::Texture2DRTVDesc
        {
            .format     = RHI::Format::Unknown,
            .mipLevel   = 0,
            .arrayLayer = 0
        });

        commandBuffer->Begin();

        commandBuffer->ExecuteBarriers(RHI::TextureTransitionBarrier
        {
            .texture       = image,
            .subresources  = { .mipLevel = 0, .arrayLayer = 0 },
            .beforeLayout  = RHI::TextureLayout::Present,
            .afterStages   = RHI::PipelineStage::RenderTarget,
            .afterAccesses = RHI::ResourceAccess::RenderTargetWrite,
            .afterLayout   = RHI::TextureLayout::RenderTarget
        }, uploadTextureAcquireBarriers, uploadBufferAcquireBarriers);

        uploadTextureAcquireBarriers.clear();
        uploadBufferAcquireBarriers.clear();

        commandBuffer->BeginRenderPass(RHI::RenderPassColorAttachment
        {
            .rtv        = rtv,
            .loadOp     = RHI::AttachmentLoadOp::Clear,
            .storeOp    = RHI::AttachmentStoreOp::Store,
            .clearValue = RHI::ColorClearValue{ 0, 1, 1, 1 }
        });

        commandBuffer->BindPipeline(pipeline);

        commandBuffer->BindGroupToGraphicsPipeline(0, bindingGroup->GetRHIBindingGroup());

        commandBuffer->SetViewports(RHI::Viewport
        {
            .lowerLeftCorner = Vector2f(0.0f, 0.0f),
            .size            = Vector2f(static_cast<float>(image->Get2DDesc().width), static_cast<float>(image->Get2DDesc().height)),
            .minDepth        = 0,
            .maxDepth        = 1
        });

        commandBuffer->SetScissors(RHI::Scissor
        {
            .lowerLeftCorner = Vector2i(0, 0),
            .size = Vector2i(static_cast<int>(image->Get2DDesc().width), static_cast<int>(image->Get2DDesc().height)),
        });

        commandBuffer->Draw(6, 1, 0, 0);

        commandBuffer->EndRenderPass();

        commandBuffer->ExecuteBarriers(RHI::TextureTransitionBarrier
        {
            .texture      = image,
            .subresources = { .mipLevel = 0, .arrayLayer = 0, },
            .beforeStages   = RHI::PipelineStage::RenderTarget,
            .beforeAccesses = RHI::ResourceAccess::RenderTargetWrite,
            .beforeLayout   = RHI::TextureLayout::RenderTarget,
            .afterLayout    = RHI::TextureLayout::Present
        });

        commandBuffer->End();

        graphicsQueue->Submit(
            { swapchain->GetAcquireSemaphore(), RHI::PipelineStage::RenderTarget },
            {},
            commandBuffer,
            { swapchain->GetPresentSemaphore(), RHI::PipelineStage::RenderTarget },
            {},
            frameResources.GetFrameFence());

        swapchain->Present();
    }

    device->WaitIdle();
}

int main()
{
    EnableMemoryLeakReporter();
    try
    {
        Run();
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}