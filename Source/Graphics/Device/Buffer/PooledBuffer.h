#pragma once

#include <Graphics/Device/Buffer/StatefulBuffer.h>
#include <Core/Container/ObjectPool.h>

RTRC_BEGIN

class PooledBufferManager;

class PooledBuffer : public StatefulBuffer
{
    friend class PooledBufferManager;
};

class PooledBufferManager : public Uncopyable, public BufferManagerInterface
{
public:

    PooledBufferManager(RHI::DeviceOPtr device, DeviceSynchronizer &sync);

    RC<PooledBuffer> Create(const RHI::BufferDesc &desc);

    void _internalRelease(Buffer &buffer) override;

private:

    struct PooledRecord
    {
        RHI::BufferPtr rhiBuffer;
        BufferState state;
    };

    RHI::DeviceOPtr device_;
    DeviceSynchronizer &sync_;

    ObjectPool<RHI::BufferDesc, PooledRecord, true, true> pool_;
};

RTRC_END