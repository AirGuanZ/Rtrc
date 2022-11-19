#include <Rtrc/Graphics/Device/Buffer/Buffer.h>

RTRC_BEGIN

BufferManager::BufferManager(RHI::DevicePtr device, DeviceSynchronizer &sync)
    : device_(std::move(device)), sync_(sync)
{
    
}

RC<Buffer> BufferManager::Create(const RHI::BufferDesc &desc)
{
    auto buffer = MakeRC<Buffer>();
    auto &bufferData = GetBufferData(*buffer);
    bufferData.rhiBuffer_ = device_->CreateBuffer(desc);
    bufferData.size_ = desc.size;
    bufferData.manager_ = this;
    return buffer;
}

void BufferManager::_internalRelease(Buffer &buffer)
{
    auto &bufferData = GetBufferData(buffer);
    assert(bufferData.manager_ == this);
    sync_.OnFrameComplete([rhiBuffer = std::move(bufferData.rhiBuffer_)] {});
}

RTRC_END
