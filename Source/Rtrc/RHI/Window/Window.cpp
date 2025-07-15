#include <array>
#include <atomic>
#include <cassert>

#if RTRC_RHI_VULKAN
#include <Rtrc/RHI/Vulkan/Context/Surface.h>
#endif
#include <Rtrc/Core/ScopeGuard.h>
#include <Rtrc/Core/Unreachable.h>
#include <Rtrc/RHI/Window/NativeWindow.h>
#include <Rtrc/RHI/Window/Window.h>

#if RTRC_RHI_VULKAN
#include <volk.h>
#endif
#include <GLFW/glfw3.h>

RTRC_BEGIN

namespace WindowDetail
{

    class GLFWStandardCursors : public Uncopyable
    {
        GLFWcursor *cursors[std::to_underlying(Window::BuiltinCursorType::Count)] = { nullptr };

    public:

        ~GLFWStandardCursors()
        {
            Clear();
        }

        GLFWcursor* GetCursor(Window::BuiltinCursorType type)
        {
            if(type == Window::BuiltinCursorType::Default)
            {
                return nullptr;
            }

            const int index = std::to_underlying(type);
            if(cursors[index])
            {
                return cursors[index];
            }

            int shape;
            switch(type)
            {
            case Window::BuiltinCursorType::Arrow:                  shape = GLFW_ARROW_CURSOR;          break;
            case Window::BuiltinCursorType::LeftRightArrow:         shape = GLFW_RESIZE_EW_CURSOR;      break;
            case Window::BuiltinCursorType::UpDownArrow:            shape = GLFW_RESIZE_NS_CURSOR;      break;
            case Window::BuiltinCursorType::LeftUpRightBottomArrow: shape = GLFW_RESIZE_NWSE_CURSOR;    break;
            case Window::BuiltinCursorType::LeftDownRightUpArrow:   shape = GLFW_RESIZE_NESW_CURSOR;    break;
            case Window::BuiltinCursorType::IBeam:                  shape = GLFW_IBEAM_CURSOR;          break;
            case Window::BuiltinCursorType::Crosshair:              shape = GLFW_CROSSHAIR_CURSOR;      break;
            case Window::BuiltinCursorType::Hand:                   shape = GLFW_POINTING_HAND_CURSOR;  break;
            case Window::BuiltinCursorType::NotAllowed:             shape = GLFW_NOT_ALLOWED_CURSOR;    break;
            default: Unreachable();
            }

            cursors[index] = glfwCreateStandardCursor(shape);
            return cursors[index];
        }

        void Clear()
        {
            for(auto &cursor : cursors)
            {
                if(cursor)
                {
                    glfwDestroyCursor(cursor);
                    cursor = nullptr;
                }
            }
        }
    };

} // namespace WindowDetail

struct Window::Impl
{
    GLFWwindow *glfwWindow = nullptr;
    WindowDetail::GLFWStandardCursors glfwStandardCursors;
    std::unique_ptr<WindowInput> input;
    Sender<WindowCloseEvent, WindowResizeEvent, WindowFocusEvent> sender;
    bool hasFocus = true;
};

namespace WindowDetail
{
    
    struct GLFWKeyCodeTable
    {
        std::array<KeyCode, GLFW_KEY_LAST + 1> keys;

        GLFWKeyCodeTable()
            : keys{}
        {
            for(auto &k : keys)
            {
                k = KeyCode::Unknown;
            }

            keys[GLFW_KEY_SPACE]      = KeyCode::Space;
            keys[GLFW_KEY_APOSTROPHE] = KeyCode::Apostrophe;
            keys[GLFW_KEY_COMMA]      = KeyCode::Comma;
            keys[GLFW_KEY_MINUS]      = KeyCode::Minus;
            keys[GLFW_KEY_PERIOD]     = KeyCode::Period;
            keys[GLFW_KEY_SLASH]      = KeyCode::Slash;

            keys[GLFW_KEY_SEMICOLON] = KeyCode::Semicolon;
            keys[GLFW_KEY_EQUAL]     = KeyCode::Equal;

            keys[GLFW_KEY_LEFT_BRACKET]  = KeyCode::LeftBracket;
            keys[GLFW_KEY_BACKSLASH]     = KeyCode::Backslash;
            keys[GLFW_KEY_RIGHT_BRACKET] = KeyCode::RightBracket;
            keys[GLFW_KEY_GRAVE_ACCENT]  = KeyCode::GraveAccent;
            keys[GLFW_KEY_ESCAPE]        = KeyCode::Escape;

            keys[GLFW_KEY_ENTER]     = KeyCode::Enter;
            keys[GLFW_KEY_TAB]       = KeyCode::Tab;
            keys[GLFW_KEY_BACKSPACE] = KeyCode::Backspace;
            keys[GLFW_KEY_INSERT]    = KeyCode::Insert;
            keys[GLFW_KEY_DELETE]    = KeyCode::Delete;

            keys[GLFW_KEY_RIGHT] = KeyCode::Right;
            keys[GLFW_KEY_LEFT]  = KeyCode::Left;
            keys[GLFW_KEY_DOWN]  = KeyCode::Down;
            keys[GLFW_KEY_UP]    = KeyCode::Up;

            keys[GLFW_KEY_HOME] = KeyCode::Home;
            keys[GLFW_KEY_END]  = KeyCode::End;

            keys[GLFW_KEY_F1]  = KeyCode::F1;
            keys[GLFW_KEY_F2]  = KeyCode::F2;
            keys[GLFW_KEY_F3]  = KeyCode::F3;
            keys[GLFW_KEY_F4]  = KeyCode::F4;
            keys[GLFW_KEY_F5]  = KeyCode::F5;
            keys[GLFW_KEY_F6]  = KeyCode::F6;
            keys[GLFW_KEY_F7]  = KeyCode::F7;
            keys[GLFW_KEY_F8]  = KeyCode::F8;
            keys[GLFW_KEY_F9]  = KeyCode::F9;
            keys[GLFW_KEY_F10] = KeyCode::F10;
            keys[GLFW_KEY_F11] = KeyCode::F11;
            keys[GLFW_KEY_F12] = KeyCode::F12;

            keys[GLFW_KEY_KP_0] = KeyCode::NumPad0;
            keys[GLFW_KEY_KP_1] = KeyCode::NumPad1;
            keys[GLFW_KEY_KP_2] = KeyCode::NumPad2;
            keys[GLFW_KEY_KP_3] = KeyCode::NumPad3;
            keys[GLFW_KEY_KP_4] = KeyCode::NumPad4;
            keys[GLFW_KEY_KP_5] = KeyCode::NumPad5;
            keys[GLFW_KEY_KP_6] = KeyCode::NumPad6;
            keys[GLFW_KEY_KP_7] = KeyCode::NumPad7;
            keys[GLFW_KEY_KP_8] = KeyCode::NumPad8;
            keys[GLFW_KEY_KP_9] = KeyCode::NumPad9;

            keys[GLFW_KEY_KP_DECIMAL]  = KeyCode::NumPadDemical;
            keys[GLFW_KEY_KP_DIVIDE]   = KeyCode::NumPadDiv;
            keys[GLFW_KEY_KP_MULTIPLY] = KeyCode::NumPadMul;
            keys[GLFW_KEY_KP_SUBTRACT] = KeyCode::NumPadSub;
            keys[GLFW_KEY_KP_ADD]      = KeyCode::NumPadAdd;
            keys[GLFW_KEY_KP_ENTER]    = KeyCode::NumPadEnter;

            keys[GLFW_KEY_LEFT_SHIFT]    = KeyCode::LeftShift;
            keys[GLFW_KEY_LEFT_CONTROL]  = KeyCode::LeftCtrl;
            keys[GLFW_KEY_LEFT_ALT]      = KeyCode::LeftAlt;
            keys[GLFW_KEY_RIGHT_SHIFT]   = KeyCode::RightShift;
            keys[GLFW_KEY_RIGHT_CONTROL] = KeyCode::RightCtrl;
            keys[GLFW_KEY_RIGHT_ALT]     = KeyCode::RightAlt;

            for(int i = 0; i < 9; ++i)
            {
                keys['0' + i] = static_cast<KeyCode>(static_cast<int>(KeyCode::D0) + i);
            }

            for(int i = 0; i < 26; ++i)
            {
                keys['A' + i] = static_cast<KeyCode>(static_cast<int>(KeyCode::A) + i);
            }
        }
    };

    struct GLFWDestroyer
    {
        bool initialized = false;

        ~GLFWDestroyer()
        {
            if(initialized)
            {
                glfwTerminate();
            }
        }
    };

    GLFWDestroyer glfwDestroyer;

    std::set<WindowInput *> windowInputs;

    void InitGLFW()
    {
        if(!glfwDestroyer.initialized)
        {
            if(glfwInit() != GLFW_TRUE)
            {
                throw Exception("failed to initialize glfw");
            }
            glfwDestroyer.initialized = true;
        }
    }

    void InitGLFWVulkan()
    {
#if RTRC_RHI_VULKAN
        RHI::InitializeVulkanBackend();
        glfwInitVulkanLoader(vkGetInstanceProcAddr);
#endif
    }
    
    void GLFWWindowCloseCallback(GLFWwindow *window)
    {
        if(auto impl = static_cast<Window::Impl *>(glfwGetWindowUserPointer(window)))
        {
            impl->sender.Send(WindowCloseEvent{});
        }
    }

    void GLFWWindowResizeCallback(GLFWwindow *window, int width, int height)
    {
        if(auto impl = static_cast<Window::Impl *>(glfwGetWindowUserPointer(window)))
        {
            impl->sender.Send(WindowResizeEvent{ width, height });
        }
    }

    void GLFWScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
    {
        if(auto impl = static_cast<Window::Impl *>(glfwGetWindowUserPointer(window)))
        {
            impl->input->_internalTriggerWheelScroll(static_cast<int>(yoffset));
        }
    }

    void GLFWMouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
    {
        auto impl = static_cast<Window::Impl *>(glfwGetWindowUserPointer(window));
        if(!impl)
        {
            return;
        }

        KeyCode keycode;
        if(button == GLFW_MOUSE_BUTTON_LEFT)
        {
            keycode = KeyCode::MouseLeft;
        }
        else if(button == GLFW_MOUSE_BUTTON_MIDDLE)
        {
            keycode = KeyCode::MouseMiddle;
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            keycode = KeyCode::MouseRight;
        }
        else
        {
            return;
        }

        if(action == GLFW_PRESS)
        {
            impl->input->_internalTriggerKeyDown(keycode);
        }
        else if(action == GLFW_RELEASE)
        {
            impl->input->_internalTriggerKeyUp(keycode);
        }
    }

    void GLFWKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        auto impl = static_cast<Window::Impl *>(glfwGetWindowUserPointer(window));
        if(!impl)
        {
            return;
        }

        static const GLFWKeyCodeTable keyCodeTable;
        if(key < 0 || key >= static_cast<int>(keyCodeTable.keys.size()))
        {
            return;
        }

        const KeyCode keycode = keyCodeTable.keys[key];
        if(keycode == KeyCode::Unknown)
        {
            return;
        }

        if(action == GLFW_PRESS)
        {
            impl->input->_internalTriggerKeyDown(keycode);
        }
        else if(action == GLFW_RELEASE)
        {
            impl->input->_internalTriggerKeyUp(keycode);
        }
    }

    void GLFWCharInputCallback(GLFWwindow *window, unsigned int ch)
    {
        if(auto impl = static_cast<Window::Impl *>(glfwGetWindowUserPointer(window)))
        {
            impl->input->_internalTriggerCharInput(ch);
        }
    }

    void GLFWWindowFocusCallback(GLFWwindow *window, int focused)
    {
        if(auto impl = static_cast<Window::Impl *>(glfwGetWindowUserPointer(window)))
        {
            const bool hasFocus = focused != 0;
            if(hasFocus != impl->hasFocus)
            {
                impl->hasFocus = hasFocus;
                impl->sender.Send(WindowFocusEvent{ hasFocus });
            }
        }
    }

} // namespace WindowDetail

void Window::DoEvents()
{
    for(auto input : WindowDetail::windowInputs)
    {
        input->_internalUpdate();
    }
    glfwPollEvents();
}

std::vector<std::string> Window::GetRequiredVulkanInstanceExtensions()
{
#if RTRC_RHI_VULKAN
    WindowDetail::InitGLFW();
    WindowDetail::InitGLFWVulkan();

    uint32_t count;
    const char **exts = glfwGetRequiredInstanceExtensions(&count);
    if(!count)
    {
        throw Exception("failed to get vulkan instance extensions for window surface");
    }

    std::vector<std::string> ret;
    for(uint32_t i = 0; i < count; ++i)
    {
        ret.emplace_back(exts[i]);
    }
    return ret;
#else
    throw Exception("vulkan backend is not supported");
#endif
}

Window::Window()
{
    
}

Window::Window(Window &&other) noexcept
    : Window()
{
    impl_.swap(other.impl_);
}

Window &Window::operator=(Window &&other) noexcept
{
    impl_.swap(other.impl_);
    return *this;
}

Window::~Window()
{
    if(!impl_)
    {
        return;
    }
    assert(impl_->glfwWindow);
    impl_->glfwStandardCursors.Clear();
    glfwDestroyWindow(impl_->glfwWindow);
    WindowDetail::windowInputs.erase(impl_->input.get());
    impl_.reset();
}

bool Window::IsInitialized() const
{
    return impl_ != nullptr;
}

void Window::SetTitle(const std::string &newTitle)
{
    glfwSetWindowTitle(impl_->glfwWindow, newTitle.c_str());
}

bool Window::ShouldClose() const
{
    assert(impl_);
    return glfwWindowShouldClose(impl_->glfwWindow);
}

void Window::SetCloseFlag(bool flag)
{
    assert(impl_);
    glfwSetWindowShouldClose(impl_->glfwWindow, flag ? 1 : 0);
}

WindowInput &Window::GetInput() const
{
    assert(impl_);
    return *impl_->input;
}

Vector2i Window::GetWindowSize() const
{
    int w, h;
    glfwGetWindowSize(impl_->glfwWindow, &w, &h);
    return { w, h };
}

Vector2i Window::GetFramebufferSize() const
{
    int w, h;
    glfwGetFramebufferSize(impl_->glfwWindow, &w, &h);
    return { w, h };
}

float Window::GetWindowWOverH() const
{
    const Vector2i size = GetWindowSize();
    return static_cast<float>(size.x) / size.y;
}

float Window::GetFramebufferWOverH() const
{
    const Vector2i size = GetFramebufferSize();
    return static_cast<float>(size.x) / size.y;
}

bool Window::HasFocus() const
{
    return impl_->hasFocus;
}

void Window::SetFocus()
{
    glfwFocusWindow(impl_->glfwWindow);
}

ReferenceCountedPtr<RHI::Surface> Window::CreateVulkanSurface(void *vkInstance)
{
#if RTRC_RHI_VULKAN
    WindowDetail::InitGLFW();
    WindowDetail::InitGLFWVulkan();

    VkSurfaceKHR surface;
    auto instance = static_cast<VkInstance>(vkInstance);
    RTRC_VK_CHECK(glfwCreateWindowSurface(instance, impl_->glfwWindow, RTRC_VK_ALLOC, &surface))
    {
        throw Exception("failed to create vulkan surface");
    };
    return MakeReferenceCountedPtr<RHI::Vk::VulkanSurface>(instance, surface);
#else
    throw Exception("vulkan backend is not supported");
#endif
}

#if RTRC_IS_WIN32

uint64_t Window::GetWin32WindowHandle() const
{
    return GetWin32WindowHandleFromGlfwWindow(impl_->glfwWindow);
}

#endif

RTRC_DEFINE_EVENT_SENDER(Window, impl_->sender, WindowCloseEvent)
RTRC_DEFINE_EVENT_SENDER(Window, impl_->sender, WindowResizeEvent)
RTRC_DEFINE_EVENT_SENDER(Window, impl_->sender, WindowFocusEvent)

void Window::SetCursor(BuiltinCursorType cursorType)
{
    assert(impl_);
    auto cursor = impl_->glfwStandardCursors.GetCursor(cursorType);
    glfwSetCursor(impl_->glfwWindow, cursor);
}

Window::Window(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl))
{
    assert(impl_);
}

WindowBuilder &WindowBuilder::SetSize(int width, int height)
{
    width_ = width;
    height_ = height;
    return *this;
}

WindowBuilder &WindowBuilder::SetMaximized(bool maximized)
{
    maximized_ = maximized;
    return *this;
}

WindowBuilder &WindowBuilder::SetTitle(std::string title)
{
    title_ = std::move(title);
    return *this;
}

Window WindowBuilder::Create() const
{
    using namespace WindowDetail;

    InitGLFW();

    // create window

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_MAXIMIZED, maximized_ ? 1 : 0);
    GLFWwindow *glfwWindow = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
    if(!glfwWindow)
    {
        throw Exception("failed to create glfw window");
    }
    RTRC_SCOPE_FAIL{ glfwDestroyWindow(glfwWindow); };

    // bind window <-> impl

    auto impl = std::make_unique<Window::Impl>();
    impl->glfwWindow = glfwWindow;
    glfwSetWindowUserPointer(glfwWindow, impl.get());

    // set event callbacks

    glfwSetWindowCloseCallback(glfwWindow, GLFWWindowCloseCallback);
    glfwSetFramebufferSizeCallback(glfwWindow, GLFWWindowResizeCallback);
    glfwSetScrollCallback(glfwWindow, GLFWScrollCallback);
    glfwSetMouseButtonCallback(glfwWindow, GLFWMouseButtonCallback);
    glfwSetKeyCallback(glfwWindow, GLFWKeyCallback);
    glfwSetCharCallback(glfwWindow, GLFWCharInputCallback);
    glfwSetWindowFocusCallback(glfwWindow, GLFWWindowFocusCallback);

    // initial state

    impl->hasFocus = glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED);

    // input manager

    impl->input = std::unique_ptr<WindowInput>(new WindowInput(glfwWindow));
    windowInputs.insert(impl->input.get());

    return Window(std::move(impl));
}

RTRC_END
