#pragma once

#include <Rtrc/Graphics/Device/Device.h>

RTRC_BEGIN

class FixedSizedTransientConstantBufferBindingGroupPool : public Uncopyable
{
public:

    struct Record
    {
        RC<SubBuffer> cbuffer;
        RC<BindingGroup> bindingGroup;
    };

    FixedSizedTransientConstantBufferBindingGroupPool(
        size_t               elementSize,
        std::string          bindingName,
        RHI::ShaderStageFlags bindingShaderStages,
        Device              &device);

    void NewBatch();

    Record NewRecord(const void *cbufferData, size_t bytes = 0);

    template<RtrcStruct T>
    Record NewRecord(const T &cbufferData);

    void Flush();

private:

    struct Batch
    {
        RC<Buffer> buffer;
        unsigned char *mappedBuffer = nullptr;
        std::vector<RC<BindingGroup>> bindingGroups;

        ~Batch();
    };

    struct SharedData
    {
        std::vector<Box<Batch>> freeBatches;
    };

    Device &device_;

    std::string          bindingName_;
    RHI::ShaderStageFlags shaderStages_;

    RC<BindingGroupLayout> bindingGroupLayout_;
    RC<SharedData>         sharedData_;

    size_t size_;
    size_t alignment_;
    size_t alignedSize_;
    size_t chunkSize_;
    size_t batchSize_;

    Batch *activeBatch_;
    size_t nextOffset_;
    size_t nextIndex_;
    size_t unflushedOffset_;
};

template<RtrcStruct T>
FixedSizedTransientConstantBufferBindingGroupPool::Record
    FixedSizedTransientConstantBufferBindingGroupPool::NewRecord(const T &cbufferData)
{
    return Rtrc::FlattenConstantBufferStruct(cbufferData, [&](const void *flattenData, size_t deviceSize)
    {
        assert(deviceSize <= size_);
        return NewRecord(flattenData, deviceSize);
    });
}

RTRC_END
