#pragma once

#include <tbb/concurrent_queue.h>

#include <Rtrc/Utility/Variant.h>
#include <Rtrc/Window/Window.h>

RTRC_BEGIN

using InputEvent = Variant<
    WindowCloseEvent,
    WindowResizeEvent,
    WindowFocusEvent,
    CursorMoveEvent,
    WheelScrollEvent,
    KeyDownEvent,
    KeyUpEvent,
    CharInputEvent>;

class InputManager :
    public Receiver<WindowCloseEvent>,
    public Receiver<WindowResizeEvent>,
    public Receiver<WindowFocusEvent>,
    public Receiver<CursorMoveEvent>,
    public Receiver<WheelScrollEvent>,
    public Receiver<KeyDownEvent>,
    public Receiver<KeyUpEvent>,
    public Receiver<CharInputEvent>
{
public:

    InputManager();
    
    void Update();
    
    RTRC_DECLARE_EVENT_SENDER(WindowCloseEvent)
    RTRC_DECLARE_EVENT_SENDER(WindowResizeEvent)
    RTRC_DECLARE_EVENT_SENDER(WindowFocusEvent)
    RTRC_DECLARE_EVENT_SENDER(CursorMoveEvent)
    RTRC_DECLARE_EVENT_SENDER(WheelScrollEvent)
    RTRC_DECLARE_EVENT_SENDER(KeyDownEvent)
    RTRC_DECLARE_EVENT_SENDER(KeyUpEvent)
    RTRC_DECLARE_EVENT_SENDER(CharInputEvent)
    
    bool IsKeyPressed(KeyCode key) const { return pressed_[std::to_underlying(key)]; }
    bool IsKeyDown   (KeyCode key) const { return down_[std::to_underlying(key)]; }
    bool IsKeyUp     (KeyCode key) const { return up_[std::to_underlying(key)]; }

    float GetCursorAbsolutePositionX() const { return absoluteX_; }
    float GetCursorAbsolutePositionY() const { return absoluteY_; }

    float GetCursorRelativePositionX() const { return relativeX_; }
    float GetCursorRelativePositionY() const { return relativeY_; }

    void LockCursor(bool lock);
    bool IsCursorLocked() const { return isLocked_; }

    bool TryPopLockCursorRequest(bool &shouldLock) { return pendingCursorLockRequests_.try_pop(shouldLock); }

private:

    using KeyStates = std::bitset<std::to_underlying(KeyCode::MaxValue) + 1>;

    void Handle(const WindowCloseEvent  &e) override { pendingEvents_.push(e); }
    void Handle(const WindowResizeEvent &e) override { pendingEvents_.push(e); }
    void Handle(const WindowFocusEvent  &e) override { pendingEvents_.push(e); }
    void Handle(const CursorMoveEvent   &e) override { pendingEvents_.push(e); }
    void Handle(const WheelScrollEvent  &e) override { pendingEvents_.push(e); }
    void Handle(const KeyDownEvent      &e) override { pendingEvents_.push(e); }
    void Handle(const KeyUpEvent        &e) override { pendingEvents_.push(e); }
    void Handle(const CharInputEvent    &e) override { pendingEvents_.push(e); }

    // Input states

    bool  isLocked_;
    float absoluteX_;
    float absoluteY_;
    float relativeX_;
    float relativeY_;

    int scrollDelta_;
    
    KeyStates pressed_;
    KeyStates down_;
    KeyStates up_;

    Sender<
        WindowCloseEvent,
        WindowResizeEvent,
        WindowFocusEvent,
        CursorMoveEvent,
        WheelScrollEvent,
        KeyDownEvent,
        KeyUpEvent,
        CharInputEvent> eventSender_;

    // Pending input events

    tbb::concurrent_queue<InputEvent> pendingEvents_;

    // Pending cursor lock request

    tbb::concurrent_queue<bool> pendingCursorLockRequests_;
};

RTRC_END
