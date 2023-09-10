#pragma once

#include <Rtrc/Graphics/Device/Device.h>
#include <Core/SmartPointer/ObserverPtr.h>

RTRC_BEGIN

enum class UploadBufferPoolPolicy
{
    LimitPoolSize,        // The pool will never grow beyond the specified max size. Smallest buffers are released first.
                          // Default pool size is 1.
    ReplaceSmallestBuffer // The pool never release buffers unless all free buffers are smaller than the required size.
                          // In that case, the smallest buffer will be replaced by a new one to satisfy the request.
};

namespace UploadBufferPoolDetail
{

    template<UploadBufferPoolPolicy Policy>
    class UploadBufferPoolPolicyData
    {
        
    };

    template<>
    class UploadBufferPoolPolicyData<UploadBufferPoolPolicy::LimitPoolSize>
    {
    protected:

        uint32_t maxPoolSize_ = 1;

    public:

        void SetMaxPoolSize(uint32_t poolSize)
        {
            maxPoolSize_ = poolSize;
        }
    };

} // namespace UploadBufferPoolDetail

template<UploadBufferPoolPolicy Policy = UploadBufferPoolPolicy::ReplaceSmallestBuffer>
class UploadBufferPool : public UploadBufferPoolDetail::UploadBufferPoolPolicyData<Policy>, public Uncopyable
{
public:
    
    UploadBufferPool(ObserverPtr<Device> device, RHI::BufferUsageFlag usages);

    RC<Buffer> Acquire(size_t leastSize);

private:

    struct SharedData
    {
        std::multimap<size_t, RC<Buffer>> freeBuffers;
    };

    void RegisterReleaseCallback(RC<Buffer> buffer);

    ObserverPtr<Device>  device_;
    RHI::BufferUsageFlag usages_;
    RC<SharedData>       sharedData_;
};

RTRC_END
