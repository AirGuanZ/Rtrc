#include <Rtrc/Renderer/TlasPool.h>

RTRC_RENDERER_BEGIN

TlasPool::TlasPool(ObserverPtr<Device> device)
    : device_(std::move(device))
{
    
}

void TlasPool::NewFrame()
{
    std::multimap<size_t, Record> newBufferSizeToRecord;
    for(auto &[bufferSize, record] : bufferSizeToRecord_)
    {
        if(currentFrameIndex_ - record.lastUsedFrameIndex < GC_FRAMES_THRESHOLD)
        {
            newBufferSizeToRecord.insert({ bufferSize, std::move(record) });
        }
    }

    bufferSizeToRecord_.swap(newBufferSizeToRecord);
    for(auto &tlas : allocatedTlases_)
    {
        Record record;
        record.lastUsedFrameIndex = currentFrameIndex_;
        record.tlas = std::move(tlas);

        const size_t bufferSize = tlas->GetBuffer()->GetSubBufferSize();
        bufferSizeToRecord_.insert({ bufferSize, std::move(record) });
    }

    ++currentFrameIndex_;
}

RC<Tlas> TlasPool::GetTlas(size_t leastBufferSize)
{
    auto it = bufferSizeToRecord_.lower_bound(leastBufferSize);
    if(it == bufferSizeToRecord_.end())
    {
        auto buffer = device_->CreateBuffer(RHI::BufferDesc
        {
            .size           = leastBufferSize,
            .usage          = RHI::BufferUsage::AccelerationStructure,
            .hostAccessType = RHI::BufferHostAccessType::None
        });
        auto statefulBuffer = StatefulBuffer::FromBuffer(std::move(buffer));
        auto tlas = device_->CreateTlas(std::move(statefulBuffer));
        allocatedTlases_.push_back(tlas);
        return tlas;
    }

    allocatedTlases_.push_back(it->second.tlas);
    bufferSizeToRecord_.erase(it);
    return allocatedTlases_.back();
}

size_t TlasPool::RelaxBufferSize(size_t leastSize)
{
    size_t ret = 1;
    while(ret < leastSize)
    {
        ret = ret << 1;
    }
    return ret;
}

RTRC_RENDERER_END
