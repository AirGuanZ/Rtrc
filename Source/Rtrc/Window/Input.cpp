#ifdef RTRC_RHI_VULKAN
#include <volk.h>
#endif
#include <GLFW/glfw3.h>

#include <Rtrc/Window/Input.h>

RTRC_BEGIN

Input::~Input()
{
    if(lock_)
    {
        LockCursor(false);
    }
}

void Input::Clear()
{
    lastKeys_.reset();
    currKeys_.reset();
    absoluteX_ = absoluteY_ = 0;
    relativeX_ = relativeY_ = 0;
    LockCursor(false);
}

bool Input::IsKeyPressed(KeyCode key) const
{
    return currKeys_[static_cast<int>(key)];
}

bool Input::IsKeyDown(KeyCode key) const
{
    return !lastKeys_[static_cast<int>(key)] && currKeys_[static_cast<int>(key)];
}

bool Input::IsKeyUp(KeyCode key) const
{
    return lastKeys_[static_cast<int>(key)] && !currKeys_[static_cast<int>(key)];
}

float Input::GetCursorAbsolutePositionX() const
{
    return absoluteX_;
}

float Input::GetCursorAbsolutePositionY() const
{
    return absoluteY_;
}

float Input::GetCursorRelativePositionX() const
{
    return relativeX_;
}

float Input::GetCursorRelativePositionY() const
{
    return relativeY_;
}

void Input::LockCursor(bool lock)
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

bool Input::IsCursorLocked() const
{
    return lock_;
}

RTRC_DEFINE_EVENT_SENDER(Input, sender_, CursorMoveEvent)
RTRC_DEFINE_EVENT_SENDER(Input, sender_, WheelScrollEvent)
RTRC_DEFINE_EVENT_SENDER(Input, sender_, KeyDownEvent)
RTRC_DEFINE_EVENT_SENDER(Input, sender_, KeyUpEvent)
RTRC_DEFINE_EVENT_SENDER(Input, sender_, CharInputEvent)

void Input::_InternalUpdate()
{
    auto [newX, newY] = QueryCursorPosition();
    relativeX_ = newX - absoluteX_;
    relativeY_ = newY - absoluteY_;
    absoluteX_ = newX;
    absoluteY_ = newY;

    if(relativeX_ != 0 || relativeY_ != 0)
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

void Input::_InternalTriggerWheelScroll(int offset)
{
    sender_.Send(WheelScrollEvent{ offset });
}

void Input::_InternalTriggerKeyDown(KeyCode key)
{
    currKeys_[static_cast<int>(key)] = true;
    sender_.Send(KeyDownEvent{ key });
}

void Input::_InternalTriggerKeyUp(KeyCode key)
{
    currKeys_[static_cast<int>(key)] = false;
    sender_.Send(KeyUpEvent{ key });
}

void Input::_InternalTriggerCharInput(uint32_t ch)
{
    sender_.Send(CharInputEvent{ ch });
}

Input::Input(void *glfwWindow)
{
    assert(glfwWindow);
    glfwWindow_ = glfwWindow;

    std::tie(absoluteX_, absoluteY_) = QueryCursorPosition();
    relativeX_ = 0;
    relativeY_ = 0;

    glfwSetInputMode(static_cast<GLFWwindow *>(glfwWindow), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    lock_ = false;

    lastKeys_.reset();
    currKeys_.reset();
}

std::pair<float, float> Input::QueryCursorPosition() const
{
    auto window = static_cast<GLFWwindow *>(glfwWindow_);
    double newXD, newYD;
    glfwGetCursorPos(window, &newXD, &newYD);
    const float newX = static_cast<float>(newXD);
    const float newY = static_cast<float>(newYD);
    return { newX, newY };
}

RTRC_END
