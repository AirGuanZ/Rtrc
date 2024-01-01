#include <Rtrc/Core/Timer.h>

RTRC_BEGIN

Timer::Timer()
    : deltaTime_{}, paused_(false), pausedTime_{}, frames_(0), fps_(0)
{
    Restart();
}

void Timer::Restart()
{
    startPoint_ = Clock::now();
    lastPoint_ = Clock::now();
    deltaTime_ = {};
    paused_ = false;
    pausedTime_ = {};
    secondPoint_ = Clock::now();
    frames_ = 0;
    fps_ = 0;
}

void Timer::BeginFrame()
{
    auto now = Clock::now();
    deltaTime_ = now - lastPoint_;
    lastPoint_ = now;
    ++frames_;
    if(now - secondPoint_ >= std::chrono::seconds(1))
    {
        fps_ = frames_;
        frames_ = 0;
        while(now - secondPoint_ >= std::chrono::seconds(1))
        {
            secondPoint_ += std::chrono::seconds(1);
        }
    }
}

void Timer::Pause()
{
    if(!paused_)
    {
        paused_ = true;
        pauseStartPoint_ = Clock::now();
    }
}

void Timer::Continue()
{
    if(paused_)
    {
        pausedTime_ = Clock::now() - pauseStartPoint_;
        paused_ = false;
    }
}

bool Timer::IsPaused() const
{
    return paused_;
}

double Timer::GetDeltaSeconds() const
{
    const auto us = std::chrono::duration_cast<std::chrono::microseconds>(deltaTime_).count();
    return 1e-6 * static_cast<double>(us);
}

double Timer::GetAccumulatedSeconds() const
{
    auto delta = Clock::now() - startPoint_;
    delta -= paused_ ? (Clock::now() - pauseStartPoint_) : pausedTime_;
    const auto us = std::chrono::duration_cast<std::chrono::microseconds>(delta).count();
    return (std::max)(0.0, 1e-6 * static_cast<double>(us));
}

int Timer::GetFps() const
{
    return fps_;
}

float Timer::GetDeltaSecondsF() const
{
    return static_cast<float>(GetDeltaSeconds());
}

float Timer::GetAccumulatedSecondsF() const
{
    return static_cast<float>(GetAccumulatedSeconds());
}

RTRC_END
