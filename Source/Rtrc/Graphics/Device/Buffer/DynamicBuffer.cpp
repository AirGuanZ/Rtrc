#include <Rtrc/Graphics/Device/Buffer/DynamicBuffer.h>
#include <Rtrc/Utility/Unreachable.h>

RTRC_BEGIN

DynamicBufferManager::DynamicBufferManager(RHI::DevicePtr device, DeviceSynchronizer &sync, size_t chunkSize)
    : device_(std::move(device)), sync_(sync)
{
    chunkSize = std::max(MIN_SLAB_SIZE, chunkSize);
    cbufferAlignment_ = device_->GetConstantBufferAlignment();
    sharedData_ = MakeRC<SharedData>();

    int maxSlabIndex = 0;
    while((MIN_SLAB_SIZE << maxSlabIndex) < chunkSize)
    {
        ++maxSlabIndex;
    }
    sharedData_->slabs = std::make_unique<Slab[]>(maxSlabIndex + 1);
    chunkSize_ = MIN_SLAB_SIZE << maxSlabIndex;
}

DynamicBufferManager::~DynamicBufferManager()
{
    chunks_.ForEach([](const Chunk &chunk)
    {
        chunk.buffer->GetRHIObject()->Unmap(0, chunk.buffer->GetSize());
    });
}

void DynamicBufferManager::_internalSetData(DynamicBuffer &buffer, const void *data, size_t size, bool isConstantBuffer)
{
    assert(buffer.manager_ == this);
    _internalRelease(buffer);

    // Process large buffer

    if(size > chunkSize_)
    {
        buffer.buffer_ = MakeRC<Buffer>();
        buffer.offset_ = 0;
        buffer.size_ = size;
        buffer.chunkIndex_ = 0;
        buffer.slabIndex_ = -1;
        auto &bufferData = GetBufferData(*buffer.buffer_);
        bufferData.rhiBuffer_ = device_->CreateBuffer(RHI::BufferDesc
        {
            .size = size,
            .usage = RHI::BufferUsage::VertexBuffer |
                     RHI::BufferUsage::IndexBuffer |
                     RHI::BufferUsage::ShaderConstantBuffer,
            .hostAccessType = RHI::BufferHostAccessType::SequentialWrite
        });
        bufferData.size_ = size;
        buffer.buffer_->Upload(data, 0, size);
        return;
    }

    // Compute real size

    size_t allocatedSize = std::max(size, MIN_SLAB_SIZE);
    if(isConstantBuffer)
    {
        allocatedSize = std::max(allocatedSize, cbufferAlignment_);
    }

    // Allocate

    int slabIndex;
    ComputeSlabIndex(allocatedSize, slabIndex, allocatedSize);
    Slab &slab = sharedData_->slabs[slabIndex];
    
    FreeBufferRecord freeBufferRecord;
    {
        std::lock_guard lockSlab(slab.mutex);
        if(!slab.freeRecords.empty())
        {
            freeBufferRecord = slab.freeRecords.back();
            slab.freeRecords.pop_back();
        }
        else
        {
            std::lock_guard lockChunks(chunkMutex_);

            // Create new chunk

            const int32_t newChunkIndex = chunks_.New();
            Chunk &newChunk = chunks_.At(newChunkIndex);
            newChunk.buffer = MakeRC<Buffer>();
            auto &newChunkBufferData = GetBufferData(*newChunk.buffer);
            newChunkBufferData.rhiBuffer_ = device_->CreateBuffer(RHI::BufferDesc
            {
                .size = chunkSize_,
                .usage = RHI::BufferUsage::VertexBuffer |
                         RHI::BufferUsage::IndexBuffer |
                         RHI::BufferUsage::ShaderConstantBuffer,
                .hostAccessType = RHI::BufferHostAccessType::SequentialWrite
            });
            newChunkBufferData.size_ = chunkSize_;
            newChunk.mappedBuffer = static_cast<unsigned char*>(newChunkBufferData.rhiBuffer_->Map(0, chunkSize_));

            freeBufferRecord.chunkIndex = newChunkIndex;
            freeBufferRecord.offsetInChunk = 0;
            for(size_t offset = allocatedSize; offset < chunkSize_; offset += allocatedSize)
            {
                slab.freeRecords.push_back({ newChunkIndex, static_cast<uint32_t>(offset) });
            }
        }
    }

    unsigned char *mappedChunkBuffer;
    {
        std::lock_guard lock(chunkMutex_);
        const Chunk &chunk = chunks_.At(freeBufferRecord.chunkIndex);
        buffer.buffer_ = chunk.buffer;
        mappedChunkBuffer = chunk.mappedBuffer;
    }

    buffer.offset_ = freeBufferRecord.offsetInChunk;
    buffer.size_ = size;
    buffer.chunkIndex_ = freeBufferRecord.chunkIndex;
    buffer.slabIndex_ = slabIndex;

    std::memcpy(mappedChunkBuffer + buffer.offset_, data, size);
    buffer.buffer_->GetRHIObject()->FlushAfterWrite(buffer.offset_, buffer.size_);
}

void DynamicBufferManager::_internalRelease(DynamicBuffer &buffer)
{
    if(!buffer.buffer_)
    {
        return;
    }

    if(buffer.slabIndex_ < 0)
    {
        sync_.OnFrameComplete([b = std::move(buffer.buffer_)] {});
        return;
    }

    sync_.OnFrameComplete([s = sharedData_, si = buffer.slabIndex_, ci = buffer.chunkIndex_, co = buffer.offset_]
    {
        Slab &slab = s->slabs[si];
        std::lock_guard lock(slab.mutex);
        slab.freeRecords.push_back({ ci, static_cast<uint32_t>(co) });
    });
}

void DynamicBufferManager::_internalRelease(Buffer &buffer)
{
    Unreachable();
}

void DynamicBufferManager::ComputeSlabIndex(size_t size, int &slabIndex, size_t &slabSize)
{
    assert(MIN_SLAB_SIZE <= size && size <= chunkSize_);
    slabIndex = 0;
    while((MIN_SLAB_SIZE << slabIndex) < size)
    {
        ++slabIndex;
    }
    slabSize = MIN_SLAB_SIZE << slabIndex;
}

RTRC_END
