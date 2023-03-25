#pragma once

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Utility/SmartPointer/ObserverPtr.h>

RTRC_RENDERER_BEGIN

class TlasPool
{
public:

    explicit TlasPool(ObserverPtr<Device> device);

    void NewFrame();

    RC<Tlas> GetTlas(size_t leastBufferSize);

private:

    static constexpr uint32_t GC_FRAMES_THRESHOLD = 60;

    static size_t RelaxBufferSize(size_t leastSize);

    struct Record
    {
        uint32_t lastUsedFrameIndex;
        RC<Tlas> tlas;
    };

    ObserverPtr<Device>           device_;
    uint32_t                      currentFrameIndex_ = 0;
    std::multimap<size_t, Record> bufferSizeToRecord_;
    std::vector<RC<Tlas>>         allocatedTlases_;
};

RTRC_RENDERER_END
