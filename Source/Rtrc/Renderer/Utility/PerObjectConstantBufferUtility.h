#pragma once

#include <Rtrc/Graphics/Device/Device.h>

RTRC_BEGIN

class PerObjectConstantBufferBatch
{
public:

    struct Record
    {
        RC<SubBuffer> cbuffer;
        RC<BindingGroup> bindingGroup;
    };

    PerObjectConstantBufferBatch(
        size_t               elementSize,
        std::string          bindingName,
        RHI::ShaderStageFlag bindingShaderStages,
        Device              &device);

    void NewBatch();

    Record NewRecord(const void *cbufferData);

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
    RHI::ShaderStageFlag shaderStages_;

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
PerObjectConstantBufferBatch::Record PerObjectConstantBufferBatch::NewRecord(const T &cbufferData)
{
    constexpr size_t deviceSize = ConstantBufferDetail::GetConstantBufferDWordCount<T>() * 4;
    assert(size_ == deviceSize);
    std::vector<unsigned char> deviceData(deviceSize);
    ConstantBufferDetail::ForEachFlattenMember<T>([&]<typename M>(const char *, size_t hostOffset, size_t deviceOffset)
    {
        auto dst = deviceData.data() + deviceOffset;
        auto src = reinterpret_cast<const unsigned char *>(&cbufferData) + hostOffset;
        std::memcpy(dst, src, sizeof(M));
    });
    return NewRecord(deviceData.data());
}

RTRC_END
