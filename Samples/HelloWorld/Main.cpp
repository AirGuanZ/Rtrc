#include <iostream>

#include <fmt/format.h>

#include <Rtrc/RHI/RHI.h>
#include <Rtrc/Window/Window.h>
#include <Rtrc/Utils/File.h>

#include <Rtrc/RHI/Shader/Type.h>

constexpr int DADADA_ARRAY_SIZE = 128;

$group_begin(MyGroup)
    $binding(Texture2D<float4>,       albedo)               // Texture2D<float> albedo, accessible in all stages
    $binding(Texture2D<int4>,         textureArray, 37, PS) // Texture2D<int4> textureArray[37], accessible in PS
    $binding(Buffer<float>,           myBuffer)
    $binding(Buffer<uint>,            myBufferArray, DADADA_ARRAY_SIZE, VS | PS)
    $binding(StructuredBuffer<uint4>, myStructuredBuffer, VS)
    $binding(Sampler, mySampler, All)
    $struct_begin(MyCBuffer)
        $variable(float, x)
        $variable(float, y)
        $variable(float, z)
        $variable(float, w)
    $struct_end()
    $cbuffer(MyCBuffer, myCBuffer, All)
$group_end()

void run()
{
    using namespace Rtrc;

    // window & device

    auto window = WindowBuilder()
        .SetSize(800, 600)
        .SetTitle("Hello, world!")
        .CreateWindow();
    
    auto &input = window.GetInput();

    auto instance = CreateVulkanInstance(
        RHI::VulkanInstanceDesc
        {
            .extensions = Window::GetRequiredVulkanInstanceExtensions(),
            .debugMode = true
        });

    auto device = instance->CreateDevice();

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

    auto vertexShaderCode = File::ReadBinaryFile("Asset/HelloWorld/HelloWorld.vert.spv");
    auto vertexShader = device->CreateVertexShader(vertexShaderCode.data(), vertexShaderCode.size(), "main");

    auto fragmentShaderCode = File::ReadBinaryFile("Asset/HelloWorld/HelloWorld.frag.spv");
    auto fragmentShader = device->CreateFragmentShader(fragmentShaderCode.data(), fragmentShaderCode.size(), "main");

    auto bindingLayout = device->CreateBindingLayout({});
    auto pipelineBuilder = device->CreatePipelineBuilder();
    auto pipeline = (*pipelineBuilder)
        .SetVertexShader(vertexShader)
        .SetFragmentShader(fragmentShader)
        .AddColorAttachment(swapchain->GetRenderTargetDesc().format)
        .SetBindingLayout(bindingLayout)
        .SetViewports(1)
        .SetScissors(1)
        .CreatePipeline();

    // test binding

    auto bindingGroupLayout = device->CreateBindingGroupLayout<MyGroup>();
    auto bindingGroup = bindingGroupLayout->CreateBindingGroup(false);
    auto buffer = device->CreateBuffer(RHI::BufferDesc
    {
        .size = 64,
        .usage = RHI::BufferUsage::ShaderStructuredBuffer,
        .hostAccessType = RHI::BufferHostAccessType::None
    });
    auto bufferSRV = buffer->CreateSRV(RHI::BufferSRVDesc
    {
        .format = RHI::Format::Unknown,
        .offset = 0,
        .range  = 64,
        .stride = 32
    });
    bindingGroup->ModifyMember(&MyGroup::myStructuredBuffer, bufferSRV);

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
            continue;;
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
            .beforeStages   = RHI::PipelineStage::ColorAttachmentOutput,
            .afterStages    = RHI::PipelineStage::ColorAttachmentOutput,
            .beforeAccess   = RHI::AccessType::None,
            .afterAccess    = RHI::AccessType::ColorAttachmentWrite,
            .beforeLayout   = RHI::TextureLayout::Undefined,
            .afterLayout    = RHI::TextureLayout::RenderTarget,
            .mipLevel       = 0,
            .arrayLayer     = 0
        }, {});

        commandBuffer->BeginRenderPass(RHI::RenderPassColorAttachment
        {
            .rtv        = rtv,
            .layout     = RHI::TextureLayout::RenderTarget,
            .loadOp     = RHI::AttachmentLoadOp::Clear,
            .storeOp    = RHI::AttachmentStoreOp::Store,
            .clearValue = RHI::ColorClearValue{ 0, 1, 1, 1 }
        });

        commandBuffer->BindPipeline(pipeline);

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

        commandBuffer->Draw(3, 1, 0, 0);

        commandBuffer->EndRenderPass();

        commandBuffer->ExecuteBarriers(RHI::TextureTransitionBarrier
        {
            .texture        = image,
            .aspectTypeFlag = RHI::AspectType::Color,
            .beforeStages   = RHI::PipelineStage::ColorAttachmentOutput,
            .afterStages    = RHI::PipelineStage::None,
            .beforeAccess   = RHI::AccessType::ColorAttachmentWrite,
            .afterAccess    = RHI::AccessType::None,
            .beforeLayout   = RHI::TextureLayout::RenderTarget,
            .afterLayout    = RHI::TextureLayout::Present,
            .mipLevel       = 0,
            .arrayLayer     = 0
        }, {});

        commandBuffer->End();

        auto queue = device->GetQueue(RHI::QueueType::Graphics);
        queue->Submit(
            swapchain->GetAcquireSemaphore(),
            RHI::PipelineStage::ColorAttachmentOutput,
            { commandBuffer },
            swapchain->GetPresentSemaphore(),
            RHI::PipelineStage::ColorAttachmentOutput,
            fences[frameIndex]);

        swapchain->Present();
    }

    device->WaitIdle();
}

int main()
{
#if defined(_DEBUG) || defined(DEBUG)
    run();
#else
    try
    {
        run();
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
#endif
}
