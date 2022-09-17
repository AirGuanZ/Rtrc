#pragma once

#include <Rtrc/Graphics/RHI/RHI.h>

RTRC_BEGIN

class FrameSynchronizer : public Uncopyable
{
public:

    FrameSynchronizer(RHI::DevicePtr device, int frameCount);

    ~FrameSynchronizer();

    void BeginFrame();

    int GetFrameIndex() const;

    // call queue->submit(frameFence) at the end of each frame
    RHI::FencePtr GetFrameFence() const;

    void OnGPUFrameEnd(std::function<void()> func);

private:

    struct FrameRecord
    {
        RHI::FencePtr fence;
        std::vector<std::function<void()>> completeCallbacks;
    };

    RHI::DevicePtr device_;
    int frameIndex_;
    std::vector<FrameRecord> frames_;

#if RTRC_DEBUG
    mutable bool getFrameFenceCalled_;
#endif
};

RTRC_END
