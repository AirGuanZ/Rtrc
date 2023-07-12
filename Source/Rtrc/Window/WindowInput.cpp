#ifdef RTRC_RHI_VULKAN
#include <volk.h>
#endif
#include <GLFW/glfw3.h>

#include <Rtrc/Window/WindowInput.h>

RTRC_BEGIN

WindowInput::~WindowInput()
{
    if(lock_)
    {
        LockCursor(false);
    }
}

void WindowInput::Clear()
{
    lastKeys_.reset();
    currKeys_.reset();
    absoluteX_ = absoluteY_ = 0;
    relativeX_ = relativeY_ = 0;
    relativeWheelScrollOffset_ = 0;
    LockCursor(false);
}

bool WindowInput::IsKeyPressed(KeyCode key) const
{
    return currKeys_[static_cast<int>(key)];
}

bool WindowInput::IsKeyDown(KeyCode key) const
{
    return !lastKeys_[static_cast<int>(key)] && currKeys_[static_cast<int>(key)];
}

bool WindowInput::IsKeyUp(KeyCode key) const
{
    return lastKeys_[static_cast<int>(key)] && !currKeys_[static_cast<int>(key)];
}

float WindowInput::GetCursorAbsolutePositionX() const
{
    return absoluteX_;
}

float WindowInput::GetCursorAbsolutePositionY() const
{
    return absoluteY_;
}

float WindowInput::GetCursorRelativePositionX() const
{
    return relativeX_;
}

float WindowInput::GetCursorRelativePositionY() const
{
    return relativeY_;
}

int WindowInput::GetRelativeWheelScrollOffset() const
{
    return relativeWheelScrollOffset_;
}

void WindowInput::LockCursor(bool lock)
{
    if(lock_ != lock)
    {
        auto window = static_cast<GLFWwindow *>(glfwWindow_);
        const int mode = lock ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
        glfwSetInputMode(window, GLFW_CURSOR, mode);
        lock_ = lock;
        std::tie(absoluteX_, absoluteY_) = QueryCursorPosition();
    }
}

bool WindowInput::IsCursorLocked() const
{
    return lock_;
}

RTRC_DEFINE_EVENT_SENDER(WindowInput, sender_, CursorMoveEvent)
RTRC_DEFINE_EVENT_SENDER(WindowInput, sender_, WheelScrollEvent)
RTRC_DEFINE_EVENT_SENDER(WindowInput, sender_, KeyDownEvent)
RTRC_DEFINE_EVENT_SENDER(WindowInput, sender_, KeyUpEvent)
RTRC_DEFINE_EVENT_SENDER(WindowInput, sender_, CharInputEvent)

void WindowInput::_internalUpdate()
{
    auto [newX, newY] = QueryCursorPosition();
    relativeX_ = newX - absoluteX_;
    relativeY_ = newY - absoluteY_;
    absoluteX_ = newX;
    absoluteY_ = newY;
    relativeWheelScrollOffset_ = 0;

    if(relativeX_ != 0.0f || relativeY_ != 0.0f)
    {
        sender_.Send(CursorMoveEvent{ 
            absoluteX_,
            absoluteY_,
            relativeX_,
            relativeY_
        });
    }

    lastKeys_ = currKeys_;
}

void WindowInput::_internalTriggerWheelScroll(int offset)
{
    sender_.Send(WheelScrollEvent{ offset });
    relativeWheelScrollOffset_ += offset;
}

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

RTRC_END
