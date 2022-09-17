#include <ranges>

#include <Rtrc/Graphics/Resource/ConstantBuffer.h>
#include <Rtrc/Utils/Bit.h>

RTRC_BEGIN

ConstantBuffer::ConstantBuffer()
    : mappedPtr_(nullptr), offset_(0), size_(0)
{
    
}

ConstantBuffer::ConstantBuffer(RHI::BufferPtr buffer, unsigned char *mappedPtr, size_t offset, size_t size)
    : buffer_(std::move(buffer)), mappedPtr_(mappedPtr + offset), offset_(offset), size_(size)
{

}

void ConstantBuffer::SetData(const void *data, size_t size)
{
    assert(buffer_ && size <= size_);
    std::memcpy(mappedPtr_, data, size);
    buffer_->FlushAfterWrite(offset_, size);
}

const RHI::BufferPtr &ConstantBuffer::GetBuffer() const
{
    return buffer_;
}

size_t ConstantBuffer::GetOffset() const
{
    return offset_;
}

size_t ConstantBuffer::GetSize() const
{
    return size_;
}

FrameConstantBufferAllocator::FrameConstantBufferAllocator(RHI::DevicePtr device, int frameCount, size_t chunkSize)
    : device_(std::move(device)), chunkSize_(chunkSize), frameCount_(frameCount), frameIndex_(0)
{
    alignment_ = device_->GetConstantBufferAlignment();
    frameRecords_.resize(frameCount);
}

FrameConstantBufferAllocator::~FrameConstantBufferAllocator()
{
    for(auto &frame : frameRecords_)
    {
        for(auto &r : std::ranges::views::values(frame.availableSizeToBufferRecord))
        {
            r.buffer->Unmap(0, r.buffer->GetDesc().size);
        }
    }
}

void FrameConstantBufferAllocator::BeginFrame()
{
    frameIndex_ = (frameIndex_ + 1) % frameCount_;

    std::multimap<size_t, FrameRecord::BufferRecord> newRecords;
    for(auto &r : std::ranges::views::values(frameRecords_[frameIndex_].availableSizeToBufferRecord))
    {
        const size_t size = r.buffer->GetDesc().size;
        newRecords.insert({ size, { r.buffer, 0, r.mappedPtr } });
    }
    frameRecords_[frameIndex_].availableSizeToBufferRecord.swap(newRecords);
}

Box<ConstantBuffer> FrameConstantBufferAllocator::AllocateConstantBuffer(size_t size)
{
    auto &records = frameRecords_[frameIndex_].availableSizeToBufferRecord;

    auto it = records.lower_bound(size);
    if(it == records.end())
    {
        const size_t allocatedSize = NextPowerOfTwo(static_cast<uint32_t>(std::max(size, chunkSize_)));
        FrameRecord::BufferRecord newRecord;
        newRecord.buffer = device_->CreateBuffer(RHI::BufferDesc{
            .size                 = allocatedSize,
            .usage                = RHI::BufferUsage::ShaderConstantBuffer,
            .hostAccessType       = RHI::BufferHostAccessType::SequentialWrite,
            .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
        });
        newRecord.mappedPtr = static_cast<unsigned char *>(newRecord.buffer->Map(0, allocatedSize));
        it = records.insert({ allocatedSize, std::move(newRecord) });
    }

    auto node = records.extract(it);
    FrameRecord::BufferRecord &bufferRecord = node.mapped();
    const size_t oldOffset = bufferRecord.nextOffset;
    const size_t newOffset = (oldOffset + size + alignment_ - 1) / alignment_ * alignment_;
    bufferRecord.nextOffset = newOffset;
    node.key() -= newOffset - oldOffset;
    records.insert(std::move(node));

    return MakeBox<ConstantBuffer>(bufferRecord.buffer, bufferRecord.mappedPtr, oldOffset, size);
}

RTRC_END
