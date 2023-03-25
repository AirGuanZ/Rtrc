#include <Rtrc/Graphics/Device/Buffer/Buffer.h>

RTRC_BEGIN

namespace BufferDetail
{

    template<typename T>
    class WrappedSubBuffer : public SubBuffer
    {
    public:

        WrappedSubBuffer(RC<T> buffer, size_t offset, size_t size)
        {
            buffer_ = std::move(buffer);
            fullOffset_ = offset + buffer_->GetSubBufferOffset();
            fullSize_ = size;
        }

        RC<Buffer> GetFullBuffer() override
        {
            if constexpr(std::is_base_of_v<Buffer, T>)
            {
                return buffer_;
            }
            else
            {
                static_assert(std::is_base_of_v<SubBuffer, T>);
                return buffer_->GetFullBuffer();
            }
        }

        size_t GetSubBufferOffset() const override
        {
            return fullOffset_;
        }

        size_t GetSubBufferSize() const override
        {
            return fullSize_;
        }

    private:

        RC<T> buffer_;
        size_t fullOffset_;
        size_t fullSize_;
    };

} // namespace BufferDetail

const RHI::BufferPtr &SubBuffer::GetFullBufferRHIObject()
{
    return GetFullBuffer()->GetRHIObject();
}

RHI::BufferDeviceAddress SubBuffer::GetDeviceAddress()
{
    auto start = GetFullBuffer()->GetRHIObject()->GetDeviceAddress();
    start.address += GetSubBufferOffset();
    return start;
}

RC<SubBuffer> SubBuffer::GetSubRange(RC<Buffer> buffer, size_t offset, size_t size)
{
    return MakeRC<BufferDetail::WrappedSubBuffer<Buffer>>(std::move(buffer), offset, size);
}

RC<SubBuffer> SubBuffer::GetSubRange(RC<SubBuffer> buffer, size_t offset, size_t size)
{
    return MakeRC<BufferDetail::WrappedSubBuffer<SubBuffer>>(std::move(buffer), offset, size);
}

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
