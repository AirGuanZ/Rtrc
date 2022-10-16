#include <mutex>

#include <Rtrc/Graphics/Object/Buffer.h>
#include <Rtrc/Graphics/Object/CommandBuffer.h>

RTRC_BEGIN

void Buffer::Upload(const void *data, size_t offset, size_t size)
{
    if(unsyncAccess_.stages != RHI::PipelineStage::None || unsyncAccess_.accesses != RHI::ResourceAccess::None)
    {
        throw Exception("Buffer must be externally synchronized before calling Buffer::Upload");
    }

    const auto hostAccess = rhiBuffer_->GetDesc().hostAccessType;
    if(hostAccess != RHI::BufferHostAccessType::SequentialWrite && hostAccess != RHI::BufferHostAccessType::Random)
    {
        throw Exception("Buffer::Upload: BufferHostAccessType must be 'SequentialWrite' or 'Random'");
    }

    auto p = rhiBuffer_->Map(offset, size, false);
    std::memcpy(p, data, size);
    rhiBuffer_->Unmap(offset, size, true);
}

void Buffer::Download(void *data, size_t offset, size_t size)
{
    if(!unsyncAccess_.IsEmpty())
    {
        throw Exception("Buffer must be externally synchronized before calling Buffer::Download");
    }

    const auto hostAccess = rhiBuffer_->GetDesc().hostAccessType;
    if(hostAccess != RHI::BufferHostAccessType::Random)
    {
        throw Exception("Buffer::Download: BufferHostAccessType must be 'Random'");
    }

    auto p = rhiBuffer_->Map(offset, size, true);
    std::memcpy(data, p, size);
    rhiBuffer_->Unmap(offset, size);
}

BufferManager::BufferManager(HostSynchronizer &hostSync, RHI::DevicePtr device)
    : batchReleaser_(hostSync), device_(std::move(device))
{
    batchReleaser_.SetPreNewBatchCallback([this]
    {
        std::list<ReleaseRecord> pendingReleaseBuffers;
        {
            std::lock_guard lock(pendingReleaseBuffersMutex_);
            pendingReleaseBuffers.swap(pendingReleaseBuffers_);
        }
        for(auto &b : pendingReleaseBuffers)
        {
            ReleaseImpl(std::move(b));
        }
    });

    batchReleaser_.SetReleaseCallback([this](int batchIndex, BatchReleaser::DataList &dataList)
    {
        for(auto it = reuseRecords_.begin(); it != reuseRecords_.end();)
        {
            if(it->second.releaseBatchIndex == batchIndex)
            {
                it = reuseRecords_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        std::lock_guard lock(garbageMutex_);
        for(auto &data : dataList)
        {
            if(data.rhiBuffer)
            {
                garbageBuffers_.push_back(std::move(data.rhiBuffer));
            }
        }
    });
}

RC<Buffer> BufferManager::CreateBuffer(
    size_t size, RHI::BufferUsageFlag usages, RHI::BufferHostAccessType hostAccess, bool allowReuse)
{
    const RHI::BufferDesc desc =
    {
        .size = size,
        .usage = usages,
        .hostAccessType = hostAccess
    };

    allowReuse &= batchReleaser_.GetHostSynchronizer().IsInOwnerThread();
    if(allowReuse)
    {
        ReuseRecord reuseRecord = { -1, {} };
        {
            std::lock_guard lock(reuseRecordsMutex_);
            if(auto it = reuseRecords_.find(desc); it != reuseRecords_.end())
            {
                reuseRecord = it->second;
                reuseRecords_.erase(it);
            }
        }
        ReleaseRecord &releaseRecord = *reuseRecord.releaseRecordIt;

        Buffer buf;
        buf.manager_ = this;
        buf.size_ = size;
        buf.rhiBuffer_ = std::move(releaseRecord.rhiBuffer);
        buf.unsyncAccess_ = std::move(releaseRecord.unsyncAccess);
        releaseRecord.rhiBuffer = {};
        releaseRecord.unsyncAccess = {};
        return MakeRC<Buffer>(std::move(buf));
    }

    Buffer buf;
    buf.manager_ = this;
    buf.rhiBuffer_ = device_->CreateBuffer(desc);
    buf.size_ = size;
    return MakeRC<Buffer>(std::move(buf));
}

void BufferManager::GC()
{
    std::vector<RHI::BufferPtr> garbageBuffers;
    {
        std::lock_guard lock(garbageMutex_);
        garbageBuffers.swap(garbageBuffers_);
    }
}

void BufferManager::_rtrcReleaseInternal(Buffer &buf)
{
    if(batchReleaser_.GetHostSynchronizer().IsInOwnerThread())
    {
        ReleaseImpl(ReleaseRecord{ std::move(buf.rhiBuffer_), buf.unsyncAccess_, buf.allowReuse_ });
    }
    else
    {
        std::lock_guard lock(pendingReleaseBuffersMutex_);
        pendingReleaseBuffers_.push_back({ std::move(buf.rhiBuffer_), buf.unsyncAccess_, buf.allowReuse_ });
    }
    buf.manager_ = nullptr;
}

void BufferManager::ReleaseImpl(ReleaseRecord buf)
{
    const bool allowReuse = buf.allowReuse;
    auto recordIt = batchReleaser_.AddToCurrentBatch(std::move(buf));
    const ReleaseRecord &releaseRecord = *recordIt;
    const int batchIndex = batchReleaser_.GetCurrentBatchIndex();
    if(allowReuse)
    {
        std::lock_guard lock(reuseRecordsMutex_);
        reuseRecords_.insert({ releaseRecord.rhiBuffer->GetDesc(), { batchIndex, recordIt } });
    }
}

RTRC_END
