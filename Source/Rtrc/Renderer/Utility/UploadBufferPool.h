#pragma once

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Utility/SmartPointer/ObserverPtr.h>

RTRC_BEGIN

class UploadBufferPool
{
public:
    
    UploadBufferPool(
        ObserverPtr<Device>  device,
        RHI::BufferUsageFlag usages,
        uint32_t             maxPoolSize);

    RC<Buffer> Acquire(size_t leastSize);

private:

    struct SharedData
    {
        std::multimap<size_t, RC<Buffer>> freeBuffers;
    };

    void RegisterReleaseCallback(RC<Buffer> buffer);

    ObserverPtr<Device>  device_;
    RHI::BufferUsageFlag usages_;
    uint32_t             maxPoolSize_;
    RC<SharedData>       sharedData_;
};

RTRC_END
