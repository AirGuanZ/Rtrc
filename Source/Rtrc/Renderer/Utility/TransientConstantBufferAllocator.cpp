#include <Rtrc/Renderer/Utility/TransientConstantBufferAllocator.h>
#include <Rtrc/Core/Enumerate.h>

RTRC_BEGIN

TransientConstantBufferAllocator::TransientConstantBufferAllocator(Device &device)
    : linearConstantBufferAllocator_(device)
{
    for(auto &&[index, pool] : Enumerate(bindingGroupPool_))
    {
        const size_t elemSize = MIN_BINDING_GROUP_CBUFFER_SIZE << index;
        pool = MakeBox<FixedSizedTransientConstantBufferBindingGroupPool>(
            elemSize, fmt::format("Transient{}", elemSize), RHI::ShaderStage::All, device);
    }
}

void TransientConstantBufferAllocator::NewBatch()
{
    for(auto &pool : bindingGroupPool_)
    {
        pool->NewBatch();
    }
    linearConstantBufferAllocator_.NewBatch();
}

void TransientConstantBufferAllocator::Flush()
{
    for(auto &pool : bindingGroupPool_)
    {
        pool->Flush();
    }
    linearConstantBufferAllocator_.Flush();
}

RC<SubBuffer> TransientConstantBufferAllocator::CreateConstantBuffer(const void *data, size_t size)
{
    return linearConstantBufferAllocator_.CreateConstantBuffer(data, size);
}

RC<BindingGroup> TransientConstantBufferAllocator::CreateConstantBufferBindingGroup(const void *data, size_t bytes)
{
    for(int i = 0; i < BINDING_GROUP_POOL_COUNT; ++i)
    {
        if(bytes < (MIN_BINDING_GROUP_CBUFFER_SIZE << i))
        {
            return bindingGroupPool_[i]->NewRecord(data, bytes).bindingGroup;
        }
    }
    throw Exception(fmt::format(
        "TransientConstantBufferAllocator: required constant buffer size is too large. "
        "Supported: {}, required: {}", MIN_BINDING_GROUP_CBUFFER_SIZE << (BINDING_GROUP_POOL_COUNT - 1), bytes));
}

RTRC_END
