#include "ConstantBuffer.h"

#include <shared_mutex>

#include <Rtrc/Graphics/Object/ConstantBuffer.h>
#include <Rtrc/Graphics/Object/HostSynchronizer.h>
#include <Rtrc/Utils/Bit.h>

RTRC_BEGIN

ConstantBuffer::ConstantBuffer()
    : manager_(nullptr), offset_(0), size_(0), chunkIndex_(-1), slabIndex_(-1)
{
    
}

ConstantBuffer::~ConstantBuffer()
{
    if(manager_)
    {
        manager_->_FreeInternal(*this);
    }
}

ConstantBuffer::ConstantBuffer(ConstantBuffer &&other) noexcept
    : ConstantBuffer()
{
    Swap(other);
}

ConstantBuffer &ConstantBuffer::operator=(ConstantBuffer &&other) noexcept
{
    Swap(other);
    return *this;
}

void ConstantBuffer::Swap(ConstantBuffer &other) noexcept
{
    std::swap(manager_, other.manager_);
    rhiBuffer_.Swap(other.rhiBuffer_);
    std::swap(offset_, other.offset_);
    std::swap(size_, other.size_);
}

void ConstantBuffer::SetData(const void *data, size_t size)
{
    assert(manager_);
    manager_->_SetDataInternal(*this, data, size);
}

const RHI::BufferPtr &ConstantBuffer::GetBuffer() const
{
    return rhiBuffer_;
}

size_t ConstantBuffer::GetOffset() const
{
    return offset_;
}

size_t ConstantBuffer::GetSize() const
{
    return size_;
}

ConstantBufferManager::ConstantBufferManager(HostSynchronizer &hostSync, RHI::DevicePtr device, size_t chunkSize)
    : hostSync_(hostSync), device_(std::move(device))
{
    cbufferAlignment_ = device_->GetConstantBufferAlignment();
    chunkSize_ = std::max(NextPowerOfTwo(chunkSize), cbufferAlignment_);

    int maxSlabIndex = 0;
    while((cbufferAlignment_ << maxSlabIndex) < chunkSize_)
    {
        ++maxSlabIndex;
    }
    assert((cbufferAlignment_ << maxSlabIndex) == chunkSize_);
    slabs_ = std::make_unique<Slab[]>(maxSlabIndex + 1);
}

ConstantBufferManager::~ConstantBufferManager()
{
    chunks_.ForEach([](const Chunk &chunk)
    {
        if(chunk.mappedPtr)
        {
            chunk.buffer->Unmap(0, chunk.buffer->GetDesc().size);
        }
    });
}

ConstantBuffer ConstantBufferManager::Create()
{
    ConstantBuffer ret;
    ret.manager_ = this;
    return ret;
}

void ConstantBufferManager::_SetDataInternal(ConstantBuffer &buffer, const void *data, size_t size)
{
    int newSlabIndex; size_t blockSize;
    ComputeSlabIndex(size, newSlabIndex, blockSize);

    _FreeInternal(buffer);

    // TODO: thread-local allocator

    Record record;
    {
        auto &slab = slabs_[newSlabIndex];
        std::lock_guard lock(slab.mutex);
        if(slab.freeBuffers.empty())
        {
            int newChunkIndex;
            {
                std::unique_lock chunkLock(chunkMutex_);
                newChunkIndex = chunks_.New();
            }
            Chunk &newChunk = chunks_.At(newChunkIndex);
            newChunk.buffer = device_->CreateBuffer(RHI::BufferDesc
            {
                .size = chunkSize_,
                .usage = RHI::BufferUsage::ShaderConstantBuffer,
                .hostAccessType = RHI::BufferHostAccessType::SequentialWrite,
                .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
            });
            newChunk.mappedPtr = newChunk.buffer->Map(0, chunkSize_);
            for(size_t offset = 0; offset < chunkSize_; offset += blockSize)
            {
                slab.freeBuffers.push_back({ newChunkIndex, offset });
            }
        }
        record = slab.freeBuffers.back();
        slab.freeBuffers.pop_back();
    }

    Chunk chunk;
    {
        std::shared_lock lock(chunkMutex_);
        chunk = chunks_.At(record.chunkIndex);
    }

    buffer.rhiBuffer_ = chunk.buffer;
    buffer.offset_ = record.offset;
    buffer.size_ = size;
    buffer.chunkIndex_ = record.chunkIndex;
    buffer.slabIndex_ = newSlabIndex;

    std::memcpy(static_cast<unsigned char *>(chunk.mappedPtr) + record.offset, data, size);
    buffer.rhiBuffer_->FlushAfterWrite(record.offset, size);
}

void ConstantBufferManager::_FreeInternal(ConstantBuffer &buffer)
{
    if(buffer.rhiBuffer_)
    {
        hostSync_.OnCurrentFrameComplete(
            [this, slabIndex = buffer.slabIndex_, chunkIndex = buffer.chunkIndex_, offset = buffer.offset_]
        {
            slabs_[slabIndex].freeBuffers.push_back({ chunkIndex, offset });
        });
        buffer.rhiBuffer_ = nullptr;
    }
}

void ConstantBufferManager::ComputeSlabIndex(size_t size, int &slabIndex, size_t &allocateSize) const
{
    assert(size <= chunkSize_);
    slabIndex = 0;
    allocateSize = cbufferAlignment_;
    while(allocateSize < size)
    {
        allocateSize <<= 1;
        ++slabIndex;
    }
}

RTRC_END
