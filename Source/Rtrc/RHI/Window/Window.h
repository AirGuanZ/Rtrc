#pragma once

#include <memory>
#include <string>

#include <Rtrc/Core/Math/Vector.h>
#include <Rtrc/Core/SmartPointer/ReferenceCounted.h>
#include <Rtrc/RHI/Window/WindowInput.h>

RTRC_RHI_BEGIN

#if RTRC_STATIC_RHI && RTRC_RHI_VULKAN
namespace Vk { class VulkanSurface; }
using Surface = Vk::VulkanSurface;
#else
class Surface;
#endif

RTRC_RHI_END

RTRC_BEGIN

struct WindowCloseEvent  { };
struct WindowResizeEvent { int width, height; };
struct WindowFocusEvent  { bool hasFocus; };

class Window : public Uncopyable
{
public:

    enum class BuiltinCursorType
    {
        Default,
        LeftRightArrow,
        UpDownArrow,
        LeftUpRightBottomArrow,
        LeftDownRightUpArrow,
        Count
    };

    struct Impl;

    static void DoEvents();

    static std::vector<std::string> GetRequiredVulkanInstanceExtensions();

    Window();

    Window(Window &&other) noexcept;

    Window &operator=(Window &&other) noexcept;

    ~Window();

    void SetTitle(const std::string &newTitle);

    bool IsInitialized() const;

    bool ShouldClose() const;

    void SetCloseFlag(bool flag);

    WindowInput &GetInput() const;

    Vector2i GetWindowSize() const;
    Vector2i GetFramebufferSize() const;

    float GetWindowWOverH() const;
    float GetFramebufferWOverH() const;

    bool HasFocus() const;

    void SetFocus();

    ReferenceCountedPtr<RHI::Surface> CreateVulkanSurface(void *vkInstance);

#if RTRC_IS_WIN32
    uint64_t GetWin32WindowHandle() const;
#endif

    RTRC_DECLARE_EVENT_SENDER(WindowCloseEvent)
    RTRC_DECLARE_EVENT_SENDER(WindowResizeEvent)
    RTRC_DECLARE_EVENT_SENDER(WindowFocusEvent)

    void SetCursor(BuiltinCursorType cursorType);

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

    int         width_     = 640;
    int         height_    = 480;
    bool        maximized_ = false;
    std::string title_     = "Rtrc";
};

RTRC_END
