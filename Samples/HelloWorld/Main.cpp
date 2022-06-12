#include <iostream>

#include <fmt/format.h>

#include <Rtrc/RHI/RHI.h>
#include <Rtrc/Window/Window.h>
#include <Rtrc/Utils/File.h>

#include <Rtrc/Shader/Binding.h>
#include <Rtrc/Shader/Shader.h>

constexpr int DADADA_ARRAY_SIZE = 128;

$struct_begin(MyCBuffer)
    $variable(float, x)
    $variable(float, y)
    $variable(float, z)
    $variable(float, w)
$struct_end()

$group_begin(MyGroup)
    $binding(Texture2D<float4>,         albedo)               // Texture2D<float> albedo, accessible in all stages
    $binding(Texture2D<int4>,           textureArray, 37, FS) // Texture2D<int4> textureArray[37], accessible in PS
    $binding(Buffer<float>,             myBuffer)
    $binding(Buffer<uint>,              myBufferArray, DADADA_ARRAY_SIZE, VS | FS)
    $binding(StructuredBuffer<uint4>,   myStructuredBuffer, VS)
    $binding(Sampler,                   mySampler, All)
    $binding(ConstantBuffer<MyCBuffer>, myCBuffer, All)
$group_end()

$group_begin(TestGroup)
    $binding(Buffer<float2>, VertexPositionBuffer, VS)
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

    auto shaderText = File::ReadTextFile("Asset/HelloWorld/HelloWorldShader.hlsl");

    auto shader = ShaderCompiler()
        .SetVertexShaderSource(shaderText, "VSMain")
        .SetFragmentShaderSource(shaderText, "PSMain")
        .SetDebugMode(true)
        .SetTarget(ShaderCompiler::Target::Vulkan)
        .AddBindingGroup<TestGroup>()
        .Compile(*device);

    auto bindingGroupLayout = device->CreateBindingGroupLayout<TestGroup>();
    auto bindingLayout = device->CreateBindingLayout(RHI::BindingLayoutDesc{ { bindingGroupLayout } });

    auto pipelineBuilder = device->CreatePipelineBuilder();
    auto pipeline = (*pipelineBuilder)
        .SetVertexShader(shader->GetVertexShader())
        .SetFragmentShader(shader->GetFragmentShader())
        .AddColorAttachment(swapchain->GetRenderTargetDesc().format)
        .SetBindingLayout(bindingLayout)
        .SetViewports(1)
        .SetScissors(1)
        .CreatePipeline();

    auto vertexPositionBuffer = device->CreateBuffer(RHI::BufferDesc
    {
        .size                 = sizeof(Vector2f) * 3,
        .usage                = RHI::BufferUsage::ShaderBuffer,
        .hostAccessType       = RHI::BufferHostAccessType::SequentialWrite,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });

    auto vertexPositionBufferSRV = vertexPositionBuffer->CreateSRV(RHI::BufferSRVDesc{
        .format = RHI::Format::R32G32_Float,
        .offset = 0,
        .range  = sizeof(Vector2f) * 3
    });

    {
        const Vector2f positions[] =
        {
            Vector2f(0.0f, 0.5f),
            Vector2f(0.5f, -0.5f),
            Vector2f(-0.5f, -0.5f)
        };
        auto p = vertexPositionBuffer->Map(0, sizeof(positions));
        std::memcpy(p, positions, sizeof(positions));
        vertexPositionBuffer->Unmap();
    }

    auto bindingGroup = bindingGroupLayout->CreateBindingGroup();
    bindingGroup->ModifyMember(&TestGroup::VertexPositionBuffer, vertexPositionBufferSRV);

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
        }, {}, {}, {}, {}, {});

        commandBuffer->BeginRenderPass(RHI::RenderPassColorAttachment
        {
            .rtv        = rtv,
            .loadOp     = RHI::AttachmentLoadOp::Clear,
            .storeOp    = RHI::AttachmentStoreOp::Store,
            .clearValue = RHI::ColorClearValue{ 0, 1, 1, 1 }
        });

        commandBuffer->BindPipeline(pipeline);

        commandBuffer->BindGroup<TestGroup>(bindingGroup);

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
            .mipLevel       = 0,
            .arrayLayer     = 0,
            .beforeState    = RHI::ResourceState::RenderTargetWrite,
            .afterState     = RHI::ResourceState::Present
        }, {}, {}, {}, {}, {});

        commandBuffer->End();

        auto queue = device->GetQueue(RHI::QueueType::Graphics);
        queue->Submit(
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
#if false && defined(_DEBUG) || defined(DEBUG)
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
