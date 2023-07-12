#pragma once

#include <bitset>
#include <Rtrc/Utility/Event.h>
#include <Rtrc/Utility/Uncopyable.h>
#include <Rtrc/Window/KeyCode.h>

RTRC_BEGIN

struct CursorMoveEvent  { float absoluteX, absoluteY, relativeX, relativeY; };
struct WheelScrollEvent { int relativeOffset; };
struct KeyDownEvent     { KeyCode key; };
struct KeyUpEvent       { KeyCode key; };
struct CharInputEvent   { uint32_t charCode; };

class WindowInput : public Uncopyable
{
public:

    ~WindowInput();

    void Clear();

    bool IsKeyPressed(KeyCode key) const;
    bool IsKeyDown   (KeyCode key) const;
    bool IsKeyUp     (KeyCode key) const;

    float GetCursorAbsolutePositionX() const;
    float GetCursorAbsolutePositionY() const;

    float GetCursorRelativePositionX() const;
    float GetCursorRelativePositionY() const;

    int GetRelativeWheelScrollOffset() const;

    void LockCursor(bool lock);
    bool IsCursorLocked() const;

    RTRC_DECLARE_EVENT_SENDER(CursorMoveEvent)
    RTRC_DECLARE_EVENT_SENDER(WheelScrollEvent)
    RTRC_DECLARE_EVENT_SENDER(KeyDownEvent)
    RTRC_DECLARE_EVENT_SENDER(KeyUpEvent)
    RTRC_DECLARE_EVENT_SENDER(CharInputEvent)

    void _internalUpdate();
    void _internalTriggerWheelScroll(int offset);
    void _internalTriggerKeyDown(KeyCode key);
    void _internalTriggerKeyUp(KeyCode key);
    void _internalTriggerCharInput(uint32_t ch);

private:

    using EventSender = Sender<CursorMoveEvent, WheelScrollEvent, KeyDownEvent, KeyUpEvent, CharInputEvent>;
    using KeyStates = std::bitset<static_cast<int>(KeyCode::MaxValue) + 1>;

    friend class WindowBuilder;

    explicit WindowInput(void *glfwWindow);

    std::pair<float, float> QueryCursorPosition() const;

    void *glfwWindow_;

    float absoluteX_;
    float absoluteY_;
    float relativeX_;
    float relativeY_;
    int relativeWheelScrollOffset_;

    bool lock_;

    KeyStates lastKeys_;
    KeyStates currKeys_;

    EventSender sender_;
};

RTRC_END
