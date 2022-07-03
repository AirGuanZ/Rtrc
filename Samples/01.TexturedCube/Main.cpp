#include <iostream>

#include <Rtrc/Rtrc.h>

$group_begin(TestGroup)
    $binding(Buffer<float2>,    VertexPositionBuffer, VS)
    $binding(Buffer<float2>,    VertexTexCoordBuffer, VS)
    $binding(Texture2D<float4>, MainTexture,          FS)
    $binding(Sampler,           MainSampler,          FS)
$group_end()

void Run()
{
    using namespace Rtrc;

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

    RC<RHI::Swapchain> swapchain;

    auto createSwapchain = [&]
    {
        device->WaitIdle();
        swapchain.reset();
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

    int frameIndex = 0;
    std::vector<RC<RHI::Fence>> fences;
    std::vector<RC<RHI::CommandPool>> commandPools;
    for(int i = 0; i < swapchain->GetRenderTargetCount(); ++i)
    {
        fences.push_back(device->CreateFence(true));
        commandPools.push_back(device->GetQueue(RHI::QueueType::Graphics)->CreateCommandPool());
    }

    // pipeline

    auto shaderText = File::ReadTextFile("Asset/01.TexturedQuad/Quad.hlsl");

    auto shader = ShaderCompiler()
        .SetVertexShaderSource(shaderText, "VSMain")
        .SetFragmentShaderSource(shaderText, "PSMain")
        .SetDebugMode(true)
        .SetTarget(ShaderCompiler::Target::Vulkan)
        .AddBindingGroup<TestGroup>()
        .Compile(*device);

    auto bindingGroupLayout = device->CreateBindingGroupLayout<TestGroup>();
    auto bindingLayout = device->CreateBindingLayout(RHI::BindingLayoutDesc{ { bindingGroupLayout } });

    auto pipelineBuilder = device->CreateGraphicsPipelineBuilder();
    auto pipeline = (*pipelineBuilder)
        .SetVertexShader(shader->GetVertexShader())
        .SetFragmentShader(shader->GetFragmentShader())
        .AddColorAttachment(swapchain->GetRenderTargetDesc().format)
        .SetBindingLayout(bindingLayout)
        .SetViewports(1)
        .SetScissors(1)
        .CreatePipeline();

    // uploader

    RHI::ResourceUploader uploader(device);
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
        graphicsQueue, RHI::ResourceState::BufferRead | RHI::ResourceState::VS));

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
        graphicsQueue, RHI::ResourceState::BufferRead | RHI::ResourceState::VS));

    // main texture

    auto mainTexData = ImageDynamic::Load("Asset/01.TexturedQuad/MainTexture.png");

    auto mainTex = device->CreateTexture2D(RHI::Texture2DDesc{
        .format               = RHI::Format::B8G8R8A8_UNorm,
        .width                = static_cast<uint32_t>(mainTexData.GetWidth()),
        .height               = static_cast<uint32_t>(mainTexData.GetHeight()),
        .mipLevels            = 1,
        .arraySize            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::TransferDst,
        .initialState         = RHI::ResourceState::Uninitialized,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });

    auto mainTexSRV = mainTex->Create2DSRV(RHI::Texture2DSRVDesc{
        .format         = RHI::Format::Unknown,
        .baseMipLevel   = 0,
        .levelCount     = 1,
        .baseArrayLayer = 0,
        .layerCount     = 1
    });

    uploadTextureAcquireBarriers.push_back(uploader.Upload(
        mainTex, RHI::AspectType::Color, 0, 0, mainTexData,
        graphicsQueue, RHI::ResourceState::TextureRead | RHI::ResourceState::FS));

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

    TestGroup bindingGroupStruct;
    bindingGroupStruct.VertexPositionBuffer = vertexPositionBufferSRV;
    bindingGroupStruct.VertexTexCoordBuffer = vertexTexCoordBufferSRV;
    bindingGroupStruct.MainTexture = mainTexSRV;
    bindingGroupStruct.MainSampler = mainSampler;

    auto bindingGroup = bindingGroupLayout->CreateBindingGroup();
    bindingGroup->Modify(bindingGroupStruct);

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

        frameIndex = (frameIndex + 1) % swapchain->GetRenderTargetCount();
        fences[frameIndex]->Wait();
        fences[frameIndex]->Reset();

        if(!swapchain->Acquire())
        {
            continue;
        }

        commandPools[frameIndex]->Reset();
        auto commandBuffer = commandPools[frameIndex]->NewCommandBuffer();

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
            .texture        = image,
            .aspectTypeFlag = RHI::AspectType::Color,
            .mipLevel       = 0,
            .arrayLayer     = 0,
            .beforeState    = RHI::ResourceState::Present,
            .afterState     = RHI::ResourceState::RenderTargetWrite
        }, {}, {}, uploadTextureAcquireBarriers, {}, uploadBufferAcquireBarriers);

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

        commandBuffer->BindGroup(bindingGroup);

        commandBuffer->SetViewports(RHI::Viewport
        {
            .lowerLeftCorner = Vector2f(0.0f, 0.0f),
            .size            = Vector2f(image->Get2DDesc().width, image->Get2DDesc().height),
            .minDepth        = 0,
            .maxDepth        = 1
        });

        commandBuffer->SetScissors(RHI::Scissor
        {
            .lowerLeftCorner = Vector2i(0, 0),
            .size = Vector2i(image->Get2DDesc().width, image->Get2DDesc().height),
        });

        commandBuffer->Draw(6, 1, 0, 0);

        commandBuffer->EndRenderPass();

        commandBuffer->ExecuteBarriers(RHI::TextureTransitionBarrier
        {
            .texture        = image,
            .aspectTypeFlag = RHI::AspectType::Color,
            .mipLevel       = 0,
            .arrayLayer     = 0,
            .beforeState    = RHI::ResourceState::RenderTargetWrite,
            .afterState     = RHI::ResourceState::Present
        }, {}, {}, {}, {}, {});

        commandBuffer->End();

        graphicsQueue->Submit(
            swapchain->GetAcquireSemaphore(),
            RHI::PipelineStage::ColorAttachmentOutput,
            commandBuffer,
            swapchain->GetPresentSemaphore(),
            RHI::PipelineStage::ColorAttachmentOutput,
            fences[frameIndex]);

        swapchain->Present();
    }

    device->WaitIdle();
}

int main()
{
    try
    {
        Run();
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
