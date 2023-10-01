#pragma once

#include <Graphics/Device/Device.h>
#include <Rtrc/Renderer/Utility/FixedSizedTransientConstantBufferBindingGroupPool.h>
#include <Rtrc/Renderer/Utility/LinearTransientConstantBufferAllocator.h>

RTRC_BEGIN

class TransientConstantBufferAllocator : public Uncopyable, public ConstantBufferManagerInterface
{
public:
    
    explicit TransientConstantBufferAllocator(Device &device);

    void NewBatch();
    void Flush();
    
    template<RtrcReflShaderStruct T>
    RC<SubBuffer> CreateConstantBuffer(const T &data);
    RC<SubBuffer> CreateConstantBuffer(const void *data, size_t size) override;
    
    template<RtrcReflShaderStruct T>
    RC<BindingGroup> CreateConstantBufferBindingGroup(const T &data);
    RC<BindingGroup> CreateConstantBufferBindingGroup(const void *data, size_t bytes);

private:

    // 256, 512, 1024, 2048, 4096, 8192
    static constexpr int    BINDING_GROUP_POOL_COUNT       = 6;
    static constexpr size_t MIN_BINDING_GROUP_CBUFFER_SIZE = 256;

    Box<FixedSizedTransientConstantBufferBindingGroupPool> bindingGroupPool_[BINDING_GROUP_POOL_COUNT];

    LinearTransientConstantBufferAllocator linearConstantBufferAllocator_;
};

template<RtrcReflShaderStruct T>
RC<SubBuffer> TransientConstantBufferAllocator::CreateConstantBuffer(const T &data)
{
    return ConstantBufferManagerInterface::CreateConstantBuffer(data);
}

template<RtrcReflShaderStruct T>
RC<BindingGroup> TransientConstantBufferAllocator::CreateConstantBufferBindingGroup(const T &data)
{
    return Rtrc::FlattenConstantBufferStruct(data, [this](const void *flattenData, size_t size)
    {
        return this->CreateConstantBufferBindingGroup(flattenData, size);
    });
}

RTRC_END
