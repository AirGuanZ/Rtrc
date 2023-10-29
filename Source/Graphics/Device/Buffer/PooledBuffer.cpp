#include <Graphics/Device/Buffer/PooledBuffer.h>

RTRC_BEGIN

PooledBufferManager::PooledBufferManager(RHI::DeviceOPtr device, DeviceSynchronizer &sync)
    : device_(std::move(device)), sync_(sync)
{
    
}

RC<PooledBuffer> PooledBufferManager::Create(const RHI::BufferDesc &desc)
{
    auto ret = MakeRC<PooledBuffer>();
    auto &bufferData = GetBufferData(*ret);
    bufferData.manager_ = this;
    bufferData.size_ = desc.size;
    if(auto record = pool_.Get(desc))
    {
        bufferData.rhiBuffer_ = std::move(record->rhiBuffer);
        ret->SetState(record->state);
    }
    else
    {
        bufferData.rhiBuffer_ = device_->CreateBuffer(desc);
    }
    return ret;
}

void PooledBufferManager::_internalRelease(Buffer &buffer)
{
    auto &pooledBuffer = static_cast<PooledBuffer &>(buffer);
    auto &bufferData = GetBufferData(pooledBuffer);

    auto record = MakeBox<PooledRecord>();
    record->rhiBuffer = std::move(bufferData.rhiBuffer_);
    record->state = pooledBuffer.GetState();

    const RHI::BufferDesc desc = record->rhiBuffer->GetDesc();
    auto handle = pool_.Insert(desc, std::move(record));
    sync_.OnFrameComplete([h = std::move(handle)] {});
}

RTRC_END
