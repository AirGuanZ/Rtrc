#pragma once

#include <chrono>

#include <Rtrc/Common.h>

RTRC_BEGIN

class Timer
{
    using Clock = std::conditional_t<
        std::chrono::high_resolution_clock::is_steady,
        std::chrono::high_resolution_clock,
        std::chrono::steady_clock>;

    Clock::time_point startPoint_;
    Clock::time_point lastPoint_;
    Clock::duration deltaTime_;

    bool paused_;
    Clock::time_point pauseStartPoint_;
    Clock::duration pausedTime_;

    Clock::time_point secondPoint_;
    int frames_;
    int fps_;

public:

    Timer();

    void Restart();
    void BeginFrame();

    void Pause(); // Accumulated seconds will not be counted
    void Continue();
    bool IsPaused() const;

    double GetDeltaSeconds() const;
    double GetAccumulatedSeconds() const;

    int GetFps() const;

    float GetDeltaSecondsF() const;
    float GetAccumulatedSecondsF() const;
};

RTRC_END
