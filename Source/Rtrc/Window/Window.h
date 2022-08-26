#pragma once

#include <memory>
#include <string>

#include <Rtrc/Math/Vector2.h>
#include <Rtrc/Utils/ReferenceCounted.h>
#include <Rtrc/Window/Input.h>

RTRC_RHI_BEGIN

class Surface;

RTRC_RHI_END

RTRC_BEGIN

struct WindowCloseEvent  { };
struct WindowResizeEvent { int width, height; };
struct WindowFocusEvent  { bool hasFocus; };

class Window : public Uncopyable
{
public:

    struct Impl;

    static void DoEvents();

    static std::vector<std::string> GetRequiredVulkanInstanceExtensions();

    Window();

    Window(Window &&other) noexcept;

    Window &operator=(Window &&other) noexcept;

    ~Window();

    bool IsInitialized() const;

    bool ShouldClose() const;

    void SetCloseFlag(bool flag);

    Input &GetInput() const;

    Vector2i GetFramebufferSize() const;

    bool HasFocus() const;

    ReferenceCountedPtr<RHI::Surface> CreateVulkanSurface(void *vkInstance);

    RTRC_DECLARE_EVENT_SENDER(WindowCloseEvent)
    RTRC_DECLARE_EVENT_SENDER(WindowResizeEvent)
    RTRC_DECLARE_EVENT_SENDER(WindowFocusEvent)

private:

    friend class WindowBuilder;

    explicit Window(std::unique_ptr<Impl> impl);
    
    std::unique_ptr<Impl> impl_;
};

class WindowBuilder
{
public:

    WindowBuilder &SetSize(int width, int height);

    WindowBuilder &SetMaximized(bool maximized);

    WindowBuilder &SetTitle(std::string title);

    Window Create() const;

private:

    int width_ = 640;
    int height_ = 480;
    bool maximized_ = false;
    std::string title_ = "Rtrc";
};

RTRC_END
