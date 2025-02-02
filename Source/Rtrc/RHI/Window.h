#pragma once

#include <bitset>

#include <Rtrc/Core/Event.h>
#include <Rtrc/Core/Math/Vector2.h>
#include <Rtrc/Core/SmartPointer/ReferenceCounted.h>
#include <Rtrc/Core/Uncopyable.h>

RTRC_BEGIN

using WindowRef = ReferenceCountedPtr<class Window>;

enum class KeyCode : int32_t
{
    Unknown    = -1,
    Space      = 32,
    Apostrophe = 39, // '
    Comma      = 44, // ,
    Minus      = 45, // -
    Period     = 46, // .
    Slash      = 47, // /

    // 0-9 <-> ASCII '0'-'9'

    D0 = 48,
    D1 = 49,
    D2 = 50,
    D3 = 51,
    D4 = 52,
    D5 = 53,
    D6 = 54,
    D7 = 55,
    D8 = 56,
    D9 = 57,

    Semicolon = 59, // ,
    Equal     = 61, // =

    // A-Z <-> ASCII 'A'-'Z'

    A = 65,
    B = 66,
    C = 67,
    D = 68,
    E = 69,
    F = 70,
    G = 71,
    H = 72,
    I = 73,
    J = 74,
    K = 75,
    L = 76,
    M = 77,
    N = 78,
    O = 79,
    P = 80,
    Q = 81,
    R = 82,
    S = 83,
    T = 84,
    U = 85,
    V = 86,
    W = 87,
    X = 88,
    Y = 89,
    Z = 90,

    LeftBracket  = 91, // [
    Backslash    = 92, // backslash
    RightBracket = 93, // ]
    GraveAccent  = 96, // `

    Escape    = 256,
    Enter     = 257,
    Tab       = 258,
    Backspace = 259,
    Insert    = 260,
    Delete    = 261,

    Right = 262,
    Left  = 263,
    Down  = 264,
    Up    = 265,

    Home = 268,
    End  = 269,

    F1  = 290,
    F2  = 291,
    F3  = 292,
    F4  = 293,
    F5  = 294,
    F6  = 295,
    F7  = 296,
    F8  = 297,
    F9  = 298,
    F10 = 299,
    F11 = 300,
    F12 = 301,

    NumPad0 = 320,
    NumPad1 = 321,
    NumPad2 = 322,
    NumPad3 = 323,
    NumPad4 = 324,
    NumPad5 = 325,
    NumPad6 = 326,
    NumPad7 = 327,
    NumPad8 = 328,
    NumPad9 = 329,

    NumPadDemical = 330,
    NumPadDiv = 331,
    NumPadMul = 332,
    NumPadSub = 333,
    NumPadAdd = 334,
    NumPadEnter= 335,

    LeftShift  = 340,
    LeftCtrl   = 341,
    LeftAlt    = 342,
    RightShift = 344,
    RightCtrl  = 345,
    RightAlt   = 346,

    MouseLeft   = 347,
    MouseMiddle = 348,
    MouseRight  = 349,

    MaxValue = 349,
};

struct CursorMoveEvent { float absoluteX, absoluteY, relativeX, relativeY; };
struct WheelScrollEvent { int relativeOffset; };
struct KeyDownEvent { KeyCode key; };
struct KeyUpEvent { KeyCode key; };
struct CharInputEvent { uint32_t charCode; };

struct WindowCloseEvent {};
struct WindowResizeEvent { int width, height; };
struct WindowFocusEvent { bool hasFocus; };

struct WindowDesc
{
    std::string title;

    uint32_t width = 640;
    uint32_t height = 480;

    bool vsync = true;
    bool maximized = false;
    bool fullscreen = false;
};

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

    friend class Window;

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

class Window : public ReferenceCounted, public Uncopyable
{
public:

    struct Impl;

    static WindowRef Create(const WindowDesc &desc);

    ~Window();

    static void ProcessPlatformEvents();

    bool GetCloseFlag() const;
    void SetCloseFlag(bool shouldClose);

    Vector2u GetSize() const;
    Vector2u GetFramebufferSize() const;

    double GetAspectRatio() const;
    double GetFramebufferAspectRatio() const;

    bool HasFocus() const;
    void SetFocus();

          WindowInput &GetInput();
    const WindowInput &GetInput() const;

    RTRC_DECLARE_EVENT_SENDER(WindowCloseEvent)
    RTRC_DECLARE_EVENT_SENDER(WindowResizeEvent)
    RTRC_DECLARE_EVENT_SENDER(WindowFocusEvent)

#if RTRC_IS_WIN32
    uint64_t GetWin32WindowHandle() const;
#endif

private:

    explicit Window(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_;
};

RTRC_END
