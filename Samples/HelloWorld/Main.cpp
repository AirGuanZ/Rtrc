#include <iostream>

#include <Rtrc/RHI/RHI.h>
#include <Rtrc/Window/Window.h>

int main()
{
    using namespace Rtrc;

    Window window = WindowBuilder()
        .SetSize(800, 600)
        .SetTitle("Hello, world!")
        .CreateWindow();
    
    Input &input = window.GetInput();

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
        RHI::VulkanInstanceDescription{
            .extensions = Window::GetRequiredVulkanInstanceExtensions(),
            .debugMode = true
        });

    auto device = instance->CreateDevice();

    auto swapchain = device->CreateSwapchain(
        RHI::SwapchainDescription{
            .format = RHI::TexelFormat::B8G8R8A8_UNorm,
            .imageCount = 3
        },
        window);

    while(!window.ShouldClose())
    {
        Window::DoEvents();
        if(input.IsKeyDown(KeyCode::Escape))
        {
            window.SetCloseFlag(true);
        }
    }
}
