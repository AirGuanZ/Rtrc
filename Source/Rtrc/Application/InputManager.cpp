#include <Rtrc/Application/InputManager.h>

RTRC_BEGIN

InputManager::InputManager()
    : isLocked_(false)
    , absoluteX_(0)
    , absoluteY_(0)
    , relativeX_(0)
    , relativeY_(0)
    , scrollDelta_(0)
{
    pressed_.reset();
    down_.reset();
    up_.reset();
}

void InputManager::Update()
{
    scrollDelta_ = 0;
    down_.reset();
    up_.reset();

    float newAbsoluteX = absoluteX_;
    float newAbsoluteY = absoluteY_;

    InputEvent nextEvent;
    while(pendingEvents_.try_pop(nextEvent))
    {
        nextEvent.Match(
            [&](const WindowCloseEvent &e)
            {
                eventSender_.Send(e);
            },
            [&](const WindowResizeEvent &e)
            {
                eventSender_.Send(e);
            },
            [&](const WindowFocusEvent &e)
            {
                eventSender_.Send(e);
            },
            [&](const CursorMoveEvent &e)
            {
                newAbsoluteX = e.absoluteX;
                newAbsoluteY = e.absoluteY;
            },
            [&](const WheelScrollEvent &e)
            {
                scrollDelta_ += e.relativeOffset;
                eventSender_.Send(e);
            },
            [&](const KeyDownEvent &e)
            {
                pressed_[std::to_underlying(e.key)] = true;
                down_[std::to_underlying(e.key)] = true;
                eventSender_.Send(e);
            },
            [&](const KeyUpEvent &e)
            {
                pressed_[std::to_underlying(e.key)] = false;
                up_[std::to_underlying(e.key)] = true;
                eventSender_.Send(e);
            },
            [&](const CharInputEvent &e)
            {
                eventSender_.Send(e);
            });
    }

    relativeX_ = newAbsoluteX - absoluteX_;
    relativeY_ = newAbsoluteY - absoluteY_;
    absoluteX_ = newAbsoluteX;
    absoluteY_ = newAbsoluteY;
    if(relativeX_ != 0.0f || relativeY_ != 0.0f)
    {
        eventSender_.Send(CursorMoveEvent{ absoluteX_, absoluteY_, relativeX_, relativeY_ });
    }
}

void InputManager::LockCursor(bool lock)
{
    if(isLocked_ == lock)
    {
        return;
    }
    isLocked_ = lock;
    pendingCursorLockRequests_.push(lock);
}

RTRC_END
