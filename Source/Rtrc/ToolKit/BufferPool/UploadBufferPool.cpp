#include <Rtrc/ToolKit/BufferPool/UploadBufferPool.h>

RTRC_BEGIN

template<UploadBufferPoolPolicy Policy>
UploadBufferPool<Policy>::UploadBufferPool(Ref<Device> device, RHI::BufferUsageFlag usages)
    : device_(device), usages_(usages)
{
    sharedData_ = MakeRC<SharedData>();
}

template<UploadBufferPoolPolicy Policy>
RC<Buffer> UploadBufferPool<Policy>::Acquire(size_t leastSize)
{
    auto it = sharedData_->freeBuffers.lower_bound(leastSize);
    if(it != sharedData_->freeBuffers.end())
    {
        RC<Buffer> buffer = it->second;
        sharedData_->freeBuffers.erase(it);
        RegisterReleaseCallback(buffer);
        return buffer;
    }
    
    RC<Buffer> buffer = device_->CreateBuffer(RHI::BufferDesc
    {
        .size           = leastSize,
        .usage          = usages_,
        .hostAccessType = RHI::BufferHostAccessType::Upload
    });

    RegisterReleaseCallback(buffer);
    if constexpr(Policy == UploadBufferPoolPolicy::ReplaceSmallestBuffer)
    {
        if(!sharedData_->freeBuffers.empty())
        {
            sharedData_->freeBuffers.erase(sharedData_->freeBuffers.begin());
        }
    }

    return buffer;
}

template<UploadBufferPoolPolicy Policy>
void UploadBufferPool<Policy>::RegisterReleaseCallback(RC<Buffer> buffer)
{
    if constexpr(Policy == UploadBufferPoolPolicy::LimitPoolSize)
    {
        device_->GetSynchronizer().OnFrameComplete(
            [b = std::move(buffer), s = sharedData_, c = this->maxPoolSize_]
            {
                const size_t size = b->GetSize();
                s->freeBuffers.insert({ size, std::move(b) });
                while(s->freeBuffers.size() > c)
                {
                    s->freeBuffers.erase(s->freeBuffers.begin());
                }
            });
    }
    else
    {
        device_->GetSynchronizer().OnFrameComplete(
            [b = std::move(buffer), s = sharedData_]
            {
                const size_t size = b->GetSize();
                s->freeBuffers.insert({ size, std::move(b) });
            });
    }
}

template class UploadBufferPool<UploadBufferPoolPolicy::LimitPoolSize>;
template class UploadBufferPool<UploadBufferPoolPolicy::ReplaceSmallestBuffer>;

RTRC_END
