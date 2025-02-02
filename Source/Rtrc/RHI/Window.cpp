#include <array>

#include <Rtrc/Core/ScopeGuard.h>
#include <Rtrc/RHI/Window.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

RTRC_BEGIN

void WindowInput::_internalTriggerKeyDown(KeyCode key)
{
    currKeys_[static_cast<int>(key)] = true;
    sender_.Send(KeyDownEvent{ key });
}

void WindowInput::_internalTriggerKeyUp(KeyCode key)
{
    currKeys_[static_cast<int>(key)] = false;
    sender_.Send(KeyUpEvent{ key });
}

void WindowInput::_internalTriggerCharInput(uint32_t ch)
{
    sender_.Send(CharInputEvent{ ch });
}

WindowInput::WindowInput(void *glfwWindow)
{
    assert(glfwWindow);
    glfwWindow_ = glfwWindow;

    absoluteX_ = 0;
    absoluteY_ = 0;
    relativeX_ = 0;
    relativeY_ = 0;
    relativeWheelScrollOffset_ = 0;
    std::tie(absoluteX_, absoluteY_) = QueryCursorPosition();

    glfwSetInputMode(static_cast<GLFWwindow *>(glfwWindow), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    lock_ = false;

    lastKeys_.reset();
    currKeys_.reset();
}

std::pair<float, float> WindowInput::QueryCursorPosition() const
{
    auto window = static_cast<GLFWwindow *>(glfwWindow_);
    double newXD, newYD;
    glfwGetCursorPos(window, &newXD, &newYD);
    const float newX = static_cast<float>(newXD);
    const float newY = static_cast<float>(newYD);
    return { newX, newY };
}
struct Window::Impl
{
    GLFWwindow *glfwWindow = nullptr;
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

void Window::ProcessPlatformEvents()
{
    for(auto input : WindowDetail::windowInputs)
    {
        input->_internalUpdate();
    }
    glfwPollEvents();
}

Window::~Window()
{
    if(!impl_)
    {
        return;
    }
    assert(impl_->glfwWindow);
    glfwDestroyWindow(impl_->glfwWindow);
    WindowDetail::windowInputs.erase(impl_->input.get());
    impl_.reset();
}

bool Window::GetCloseFlag() const
{
    assert(impl_);
    return glfwWindowShouldClose(impl_->glfwWindow);
}

void Window::SetCloseFlag(bool flag)
{
    assert(impl_);
    glfwSetWindowShouldClose(impl_->glfwWindow, flag ? 1 : 0);
}

WindowInput &Window::GetInput()
{
    assert(impl_);
    return *impl_->input;
}

const WindowInput &Window::GetInput() const
{
    assert(impl_);
    return *impl_->input;
}

Vector2u Window::GetSize() const
{
    int w, h;
    glfwGetWindowSize(impl_->glfwWindow, &w, &h);
    return { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
}

Vector2u Window::GetFramebufferSize() const
{
    int w, h;
    glfwGetFramebufferSize(impl_->glfwWindow, &w, &h);
    return { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
}

double Window::GetAspectRatio() const
{
    const Vector2u size = GetSize();
    return static_cast<double>(size.x) / size.y;
}

double Window::GetFramebufferAspectRatio() const
{
    const Vector2u size = GetFramebufferSize();
    return static_cast<double>(size.x) / size.y;
}

bool Window::HasFocus() const
{
    return impl_->hasFocus;
}

void Window::SetFocus()
{
    glfwFocusWindow(impl_->glfwWindow);
}

#if RTRC_IS_WIN32

uint64_t Window::GetWin32WindowHandle() const
{
    auto handle = glfwGetWin32Window(impl_->glfwWindow);
    static_assert(sizeof(handle) == sizeof(uint64_t));
    return reinterpret_cast<uint64_t>(handle);
}

#endif

RTRC_DEFINE_EVENT_SENDER(Window, impl_->sender, WindowCloseEvent)
RTRC_DEFINE_EVENT_SENDER(Window, impl_->sender, WindowResizeEvent)
RTRC_DEFINE_EVENT_SENDER(Window, impl_->sender, WindowFocusEvent)

Window::Window(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl))
{
    assert(impl_);
}

WindowRef Window::Create(const WindowDesc &desc)
{
    using namespace WindowDetail;

    InitGLFW();

    // create window

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_MAXIMIZED, desc.maximized ? 1 : 0);
    GLFWwindow *glfwWindow = glfwCreateWindow(desc.width, desc.height, desc.title.c_str(), nullptr, nullptr);
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

    auto window = new Window(std::move(impl));
    return WindowRef(window);
}

RTRC_END
