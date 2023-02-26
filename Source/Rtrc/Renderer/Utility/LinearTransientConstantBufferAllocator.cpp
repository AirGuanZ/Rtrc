#include <Rtrc/Renderer/Utility/LinearTransientConstantBufferAllocator.h>

RTRC_BEGIN

LinearTransientConstantBufferAllocator::LinearTransientConstantBufferAllocator(Device &device, size_t batchBufferSize)
    : device_(device), batchBufferSize_(batchBufferSize)
{
    constantBufferAlignment_ = device_.GetRawDevice()->GetConstantBufferAlignment();

    activeBatch_     = nullptr;
    nextOffset_      = 0;
    unflushedOffset_ = 0;

    sharedData_ = MakeRC<SharedData>();
}

void LinearTransientConstantBufferAllocator::NewBatch()
{
    Box<Batch> newBatch;
    if(!sharedData_->freeBatches.empty())
    {
        newBatch = std::move(sharedData_->freeBatches.back());
        sharedData_->freeBatches.pop_back();
    }
    else
    {
        newBatch = MakeBox<Batch>();
        newBatch->buffer = device_.CreateBuffer(RHI::BufferDesc
        {
            .size           = batchBufferSize_,
            .usage          = RHI::BufferUsage::ShaderConstantBuffer,
            .hostAccessType = RHI::BufferHostAccessType::SequentialWrite
        });
        newBatch->mappedBuffer = static_cast<unsigned char *>(
            newBatch->buffer->GetRHIObject()->Map(0, batchBufferSize_));
    }

    Batch *rawNewBatch = newBatch.get();
    device_.GetSynchronizer().OnFrameComplete([sd = this->sharedData_, b = std::move(newBatch)]() mutable
    {
        sd->freeBatches.emplace_back(std::move(b));
    });

    Flush();
    assert(unflushedOffset_ == nextOffset_);

    activeBatch_     = rawNewBatch;
    nextOffset_      = 0;
    unflushedOffset_ = 0;
}

void LinearTransientConstantBufferAllocator::Flush()
{
    if(unflushedOffset_ < nextOffset_)
    {
        activeBatch_->buffer->GetRHIObject()->FlushAfterWrite(unflushedOffset_, nextOffset_ - unflushedOffset_);
        unflushedOffset_ = nextOffset_;
    }
}

RC<SubBuffer> LinearTransientConstantBufferAllocator::CreateConstantBuffer(const void *data, size_t size)
{
    assert(data && size);

    if(size > (batchBufferSize_ >> 1))
    {
        return device_.CreateConstantBuffer(data, size);
    }

    if(!activeBatch_)
    {
        NewBatch();
    }

    const size_t alignedOffset = UpAlignTo(nextOffset_, constantBufferAlignment_);
    if(alignedOffset + size > batchBufferSize_)
    {
        NewBatch();
    }

    auto ret = SubBuffer::GetSubRange(activeBatch_->buffer, alignedOffset, size);
    std::memcpy(activeBatch_->mappedBuffer + alignedOffset, data, size);
    nextOffset_ = alignedOffset + size;
    return ret;
}

LinearTransientConstantBufferAllocator::Batch::~Batch()
{
    if(mappedBuffer)
    {
        assert(buffer);
        buffer->GetRHIObject()->Unmap(0, buffer->GetSize(), false);
    }
}

RTRC_END
