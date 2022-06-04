#include <iostream>

#include <fmt/format.h>

#include <Rtrc/RHI/RHI.h>
#include <Rtrc/Window/Window.h>
#include <Rtrc/Utils/File.h>

#include <Rtrc/RHI/Shader/Type.h>

constexpr int DADADA_ARRAY_SIZE = 128;

$group_begin(MyGroup)
    $binding(Texture2D<float4>, albedo) // Texture2D<float> albedo, accessible in all stages
    $binding(Texture2D<int4>,   textureArray, 37, PS) // Texture2D<int4> textureArray[37], accessible in VS
    $binding(Buffer<float>,     myBuffer)
    $binding(Buffer<uint>,      myBufferArray, DADADA_ARRAY_SIZE, VS | PS)
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

template<typename Struct>
void PrintCBuffer(const char *name)
{
    fmt::print("cbuffer {}\n", name);
    fmt::print("{{\n");
    Struct::template ForEachMember(
        []<typename Member>(Member Struct:: *p, const char *memberName)
    {
        fmt::print("  {} {}\n", typeid(Member).name(), memberName);
    });
    fmt::print("}}\n");
}

void test()
{
    MyGroup::ForEachMember<Rtrc::EnumFlags(Rtrc::ShaderDetail::MemberTag::Binding)>([]<typename Slot>()
    {
        if constexpr(Slot::IsArray)
        {
            fmt::print("{} {}[{}]\n", Slot::TypeName, Slot::Name, Slot::ArraySize);
        }
        else
        {
            fmt::print("{} {}\n", Slot::TypeName, Slot::Name);
        }
    });

    MyGroup::ForEachMember<Rtrc::EnumFlags(Rtrc::ShaderDetail::MemberTag::CBuffer)>(
        []<typename Struct>(Struct MyGroup:: * p, const char *name, Rtrc::EnumFlags<Rtrc::RHI::ShaderStage> stages)
    {
        PrintCBuffer<Struct>(name);
    });
}

void run()
{
    using namespace Rtrc;

    auto window = WindowBuilder()
        .SetSize(800, 600)
        .SetTitle("Hello, world!")
        .CreateWindow();
    
    auto &input = window.GetInput();

    window.Attach([&](const WindowCloseEvent &)
    {
        std::cout << "close" << std::endl;
    });

    window.Attach([&](const WindowResizeEvent &e)
    {
        std::cout << "resized: " << e.width << " " << e.height << std::endl;
    });

    window.Attach([&](const WindowFocusEvent &e)
    {
        std::cout << "focus " << e.hasFocus << std::endl;
    });

    input.Attach([&](const KeyDownEvent &e)
    {
        if(e.key == KeyCode('A'))
        {
            std::cout << "press A" << std::endl;
        }
    });

    input.Attach([&](const WheelScrollEvent &e)
    {
        std::cout << "scroll " << e.relativeOffset << std::endl;
    });

    auto instance = CreateVulkanInstance(
        RHI::VulkanInstanceDesc{
            .extensions = Window::GetRequiredVulkanInstanceExtensions(),
            .debugMode = true
        });

    auto device = instance->CreateDevice();

    auto swapchain = device->CreateSwapchain(
        RHI::SwapchainDesc{
            .format = RHI::Format::B8G8R8A8_UNorm,
            .imageCount = 3
        },
        window);

    std::vector<RC<RHI::Fence>> fences;
    std::vector<RC<RHI::CommandPool>> commandPools;
    for(int i = 0; i < swapchain->GetRenderTargetCount(); ++i)
    {
        fences.push_back(device->CreateFence(true));
        commandPools.push_back(device->GetQueue(RHI::QueueType::Graphics)->CreateCommandPool());
    }

    int frameIndex = 0;
    while(!window.ShouldClose())
    {
        Window::DoEvents();
        if(input.IsKeyDown(KeyCode::Escape))
        {
            window.SetCloseFlag(true);
        }

        frameIndex = (frameIndex + 1) % swapchain->GetRenderTargetCount();
        fences[frameIndex]->Wait();
        fences[frameIndex]->Reset();

        if(!swapchain->Acquire())
        {
            break;
        }

        commandPools[frameIndex]->Reset();
        auto commandBuffer = commandPools[frameIndex]->NewCommandBuffer();

        auto image = swapchain->GetRenderTarget();
        auto rtv = image->Create2DRTV(RHI::Texture2DRTVDesc{
            .format     = RHI::Format::Unknown,
            .mipLevel   = 0,
            .arrayLayer = 0
        });

        commandBuffer->Begin();

        commandBuffer->ExecuteBarriers({ RHI::TextureTransitionBarrier{
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
        } }, {});

        commandBuffer->BeginRenderPass({ RHI::RenderPassColorAttachment{
            .rtv        = rtv,
            .layout     = RHI::TextureLayout::RenderTarget,
            .loadOp     = RHI::AttachmentLoadOp::Clear,
            .storeOp    = RHI::AttachmentStoreOp::Store,
            .clearValue = RHI::ColorClearValue{ 0, 1, 1, 1 }
        } });

        commandBuffer->EndRenderPass();

        commandBuffer->ExecuteBarriers({ RHI::TextureTransitionBarrier{
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
        } }, {});

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
    //test();
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
