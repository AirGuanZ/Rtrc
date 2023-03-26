#include <Rtrc/Renderer/Utility/UploadBufferPool.h>

RTRC_BEGIN

UploadBufferPool::UploadBufferPool(ObserverPtr<Device> device, RHI::BufferUsageFlag usages, uint32_t maxPoolSize)
    : device_(device), usages_(usages), maxPoolSize_(maxPoolSize)
{
    sharedData_ = MakeRC<SharedData>();
}

RC<Buffer> UploadBufferPool::Acquire(size_t leastSize)
{
    auto it = sharedData_->freeBuffers.lower_bound(leastSize);
    if(it != sharedData_->freeBuffers.end())
    {
        auto buffer = it->second;
        sharedData_->freeBuffers.erase(it);
        RegisterReleaseCallback(buffer);
        return buffer;
    }
    auto buffer = device_->CreateBuffer(RHI::BufferDesc
    {
        .size           = leastSize,
        .usage          = usages_,
        .hostAccessType = RHI::BufferHostAccessType::SequentialWrite
    });
    RegisterReleaseCallback(buffer);
    return buffer;
}

void UploadBufferPool::RegisterReleaseCallback(RC<Buffer> buffer)
{
    device_->GetSynchronizer().OnFrameComplete(
        [b = std::move(buffer), s = sharedData_, c = maxPoolSize_]
        {
            const size_t size = b->GetSize();
            s->freeBuffers.insert({ size, std::move(b) });
            while(s->freeBuffers.size() > c)
            {
                s->freeBuffers.erase(s->freeBuffers.begin());
            }
        });
}

RTRC_END
