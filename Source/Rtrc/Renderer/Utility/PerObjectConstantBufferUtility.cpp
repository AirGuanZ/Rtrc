#include <Rtrc/Renderer/Utility/PerObjectConstantBufferUtility.h>

RTRC_BEGIN

PerObjectConstantBufferBatch::PerObjectConstantBufferBatch(
    size_t               elementSize,
    std::string          bindingName,
    RHI::ShaderStageFlag bindingShaderStages,
    Device              &device)
    : device_(device)
{
    bindingName_ = std::move(bindingName);
    shaderStages_ = bindingShaderStages;

    bindingGroupLayout_ = device.CreateBindingGroupLayout(BindingGroupLayout::Desc
    {
        .bindings = {
            BindingGroupLayout::BindingDesc
            {
                .type = RHI::BindingType::ConstantBuffer,
                .stages = shaderStages_
            }
        }
    });
    sharedData_ = MakeRC<SharedData>();

    size_ = elementSize;
    alignment_ = device.GetRawDevice()->GetConstantBufferAlignment();
    alignedSize_ = UpAlignTo(size_, alignment_);
    if(alignedSize_ > 4 * 1024 * 1024)
    {
        throw Exception(fmt::format(
            "Aligned element size exceeds limit. Limit = {}, size = {}", 4 * 1024 * 1024, alignedSize_));
    }
    chunkSize_ = UpAlignTo(static_cast<size_t>(4 * 1024 * 1024), alignedSize_);
    batchSize_ = chunkSize_ / alignedSize_;

    activeBatch_ = nullptr;
    nextOffset_ = 0;
    nextIndex_ = 0;
    unflushedOffset_ = 0;
}

void PerObjectConstantBufferBatch::NewBatch()
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
                .size = chunkSize_,
                .usage = RHI::BufferUsage::ShaderConstantBuffer,
                .hostAccessType = RHI::BufferHostAccessType::SequentialWrite
            });
        newBatch->mappedBuffer = static_cast<unsigned char *>(newBatch->buffer->GetRHIObject()->Map(0, chunkSize_));
        newBatch->bindingGroups.resize(batchSize_);
    }

    Batch *rawNewBatch = newBatch.get();
    device_.GetSynchronizer().OnFrameComplete([sd = this->sharedData_, b = std::move(newBatch)]() mutable
    {
        sd->freeBatches.emplace_back(std::move(b));
    });

    Flush();
    assert(unflushedOffset_ == nextOffset_);

    activeBatch_ = rawNewBatch;
    nextOffset_ = 0;
    nextIndex_ = 0;
    unflushedOffset_ = 0;
}

PerObjectConstantBufferBatch::Record PerObjectConstantBufferBatch::NewRecord(const void *cbufferData)
{
    if(!activeBatch_ || nextOffset_ >= chunkSize_)
    {
        NewBatch();
    }

    if(!activeBatch_->bindingGroups[nextIndex_])
    {
        activeBatch_->bindingGroups[nextIndex_] = bindingGroupLayout_->CreateBindingGroup();
        activeBatch_->bindingGroups[nextIndex_]->Set(0, activeBatch_->buffer, nextOffset_, size_);
    }

    auto cbuffer = SubBuffer::GetSubRange(activeBatch_->buffer, nextOffset_, size_);
    std::memcpy(activeBatch_->mappedBuffer + nextOffset_, cbufferData, size_);

    Record ret;
    ret.cbuffer = std::move(cbuffer);
    ret.bindingGroup = activeBatch_->bindingGroups[nextIndex_];

    nextOffset_ += alignedSize_;
    nextIndex_ += 1;

    return ret;
}

void PerObjectConstantBufferBatch::Flush()
{
    if(unflushedOffset_ < nextOffset_)
    {
        activeBatch_->buffer->GetRHIObject()->FlushAfterWrite(unflushedOffset_, nextOffset_ - unflushedOffset_);
        unflushedOffset_ = nextOffset_;
    }
}

PerObjectConstantBufferBatch::Batch::~Batch()
{
    if(mappedBuffer)
    {
        assert(buffer);
        buffer->GetRHIObject()->Unmap(0, buffer->GetSize(), false);
    }
}

RTRC_END
