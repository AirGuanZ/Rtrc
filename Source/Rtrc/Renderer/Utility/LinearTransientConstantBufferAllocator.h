#pragma once

#include <Rtrc/Graphics/Device/Device.h>

RTRC_BEGIN

class LinearTransientConstantBufferAllocator : public ConstantBufferManagerInterface, public Uncopyable
{
public:

    LinearTransientConstantBufferAllocator(Device &device, size_t batchBufferSize = 2048);

    void NewBatch();
    void Flush();
    
    RC<SubBuffer> CreateConstantBuffer(const void *data, size_t size) override;

private:

    struct Batch
    {
        RC<Buffer>     buffer;
        unsigned char *mappedBuffer = nullptr;

        ~Batch();
    };

    struct SharedData
    {
        std::vector<Box<Batch>> freeBatches;
    };

    Device &device_;
    size_t batchBufferSize_;
    size_t constantBufferAlignment_;

    RC<SharedData> sharedData_;

    Batch *activeBatch_;
    size_t nextOffset_;
    size_t unflushedOffset_;
};

RTRC_END
