#include <ranges>

#include <Rtrc/Graphics/Resource/ConstantBuffer.h>
#include <Rtrc/Utils/Bit.h>

RTRC_BEGIN

FrameConstantBuffer::FrameConstantBuffer(RHI::BufferPtr buffer, unsigned char *mappedPtr, size_t offset, size_t size)
    : buffer_(std::move(buffer)), mappedPtr_(mappedPtr + offset), offset_(offset), size_(size)
{

}

void FrameConstantBuffer::SetData(const void *data, size_t size)
{
    assert(buffer_ && size <= size_);
    std::memcpy(mappedPtr_, data, size);
    buffer_->FlushAfterWrite(offset_, size);
}

PersistentConstantBuffer::~PersistentConstantBuffer()
{
    if(parentManager_)
    {
        parentManager_->_FreeInternal(*this);
    }
}

PersistentConstantBuffer::PersistentConstantBuffer(PersistentConstantBuffer &&other) noexcept
    : PersistentConstantBuffer()
{
    Swap(other);
}

PersistentConstantBuffer &PersistentConstantBuffer::operator=(PersistentConstantBuffer &&other) noexcept
{
    Swap(other);
    return *this;
}

void PersistentConstantBuffer::Swap(PersistentConstantBuffer &other) noexcept
{
    std::swap(parentManager_, other.parentManager_);
    buffer_.Swap(other.buffer_);
    std::swap(offset_, other.offset_);
    std::swap(size_, other.size_);
    std::swap(chunkIndex_, other.chunkIndex_);
    std::swap(slabIndex_, other.slabIndex_);
}

void PersistentConstantBuffer::SetData(const void *data, size_t size)
{
    assert(parentManager_);
    assert(size <= size_);
    auto mappedPtr = parentManager_->_AllocateInternal(*this);
    std::memcpy(mappedPtr, data, size);
    buffer_->FlushAfterWrite(offset_, size);
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

FrameConstantBuffer FrameConstantBufferAllocator::AllocateConstantBuffer(size_t size)
{
    auto &records = frameRecords_[frameIndex_].availableSizeToBufferRecord;

    auto it = records.lower_bound(size);
    if(it == records.end())
    {
        const size_t allocatedSize = NextPowerOfTwo(static_cast<uint32_t>(std::max(size, chunkSize_)));
        FrameRecord::BufferRecord newRecord;
        newRecord.buffer = device_->CreateBuffer(RHI::BufferDesc{
            .size           = allocatedSize,
            .usage          = RHI::BufferUsage::ShaderConstantBuffer,
            .hostAccessType = RHI::BufferHostAccessType::SequentialWrite
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

    return FrameConstantBuffer(bufferRecord.buffer, bufferRecord.mappedPtr, oldOffset, size);
}

PersistentConstantBufferManager::PersistentConstantBufferManager(RHI::DevicePtr device, int frameCount, size_t chunkSize)
    : device_(std::move(device)), frameCount_(frameCount)
{
    int s = 0;
    while((2u << s) <= chunkSize)
    {
        ++s;
    }
    assert((1u << s) <= chunkSize && (2u << s) > chunkSize);
    chunkSize_ = 1u << s;
    // 1 << 0, 1 << 1, 1 << 2, 1 << 3, ..., 1 << s
    slabs_.resize(s + 1);
}

PersistentConstantBufferManager::~PersistentConstantBufferManager()
{
    for(auto &c : chunks_)
    {
        c.buffer->Unmap(0, c.buffer->GetDesc().size);
    }
}

void PersistentConstantBufferManager::BeginFrame()
{
    std::vector<PendingRecord> newPendingRecords;
    for(PendingRecord &r : pendingRecords_)
    {
        if(--r.pendingFrames)
        {
            newPendingRecords.push_back(r);
            continue;
        }
        slabs_[r.slabIndex].push_back(r);
    }
    pendingRecords_ = std::move(newPendingRecords);
}

PersistentConstantBuffer PersistentConstantBufferManager::AllocateConstantBuffer(size_t size)
{
    PersistentConstantBuffer ret;
    ret.parentManager_ = this;
    ret.size_ = size;
    return ret;
}

void *PersistentConstantBufferManager::_AllocateInternal(PersistentConstantBuffer &buffer)
{
    assert(buffer.size_ <= chunkSize_);

    int slabIndex = -1;
    if(buffer.buffer_)
    {
        slabIndex = buffer.slabIndex_;

        PendingRecord record;
        record.chunkIndex = buffer.chunkIndex_;
        record.offsetInBuffer = buffer.offset_;
        record.slabIndex = buffer.slabIndex_;
        record.pendingFrames = frameCount_;
        pendingRecords_.push_back(record);
    }

    if(slabIndex < 0)
    {
        slabIndex = CalculateSlabIndex(buffer.size_);
    }

    auto slab = slabs_[slabIndex];
    if(slab.empty())
    {
        auto newBuffer = device_->CreateBuffer(RHI::BufferDesc
        {
            .size = chunkSize_,
            .usage = RHI::BufferUsage::ShaderConstantBuffer,
            .hostAccessType = RHI::BufferHostAccessType::SequentialWrite
        });

        auto mappedPtr = newBuffer->Map(0, chunkSize_);

        const int newBufferIndex = static_cast<int>(chunks_.size());
        chunks_.push_back({ std::move(newBuffer), mappedPtr });

        const size_t recordSize = 1 << slabIndex;
        for(size_t offset = 0; offset + recordSize < chunkSize_; offset += recordSize)
        {
            slab.push_back(Record{ newBufferIndex, offset });
        }
    }

    auto r = slab.back();
    slab.pop_back();

    buffer.buffer_ = chunks_[r.chunkIndex].buffer;
    buffer.offset_ = r.offsetInBuffer;
    buffer.chunkIndex_ = r.chunkIndex;
    buffer.slabIndex_ = slabIndex;

    return static_cast<unsigned char *>(chunks_[r.chunkIndex].mappedPtr) + r.offsetInBuffer;
}

void PersistentConstantBufferManager::_FreeInternal(PersistentConstantBuffer &buffer)
{
    PendingRecord record;
    record.chunkIndex = buffer.chunkIndex_;
    record.offsetInBuffer = buffer.offset_;
    record.slabIndex = buffer.slabIndex_;
    record.pendingFrames = frameCount_;
    pendingRecords_.push_back(record);

    buffer.buffer_ = nullptr;
    buffer.offset_ = 0;
    buffer.slabIndex_ = -1;
    buffer.chunkIndex_ = -1;
}

int PersistentConstantBufferManager::CalculateSlabIndex(size_t size)
{
    size = std::max(size, static_cast<size_t>(256));
    int ret = 0;
    while((1u << ret) < size)
    {
        ++ret;
    }
    return ret;
}

RTRC_END
