#pragma once

#include <mutex>
#include <shared_mutex>

#include <Rtrc/Graphics/Device/Buffer/Buffer.h>
#include <Rtrc/Graphics/Device/Buffer/CBufferStruct.h>
#include <Rtrc/Core/Container/SlotVector.h>
#include <Rtrc/Core/Swap.h>

RTRC_BEGIN

class ConstantBufferManagerInterface
{
public:

    virtual ~ConstantBufferManagerInterface() = default;

    virtual RC<SubBuffer> CreateConstantBuffer(const void *data, size_t size) = 0;

    template<RtrcStruct T>
    RC<SubBuffer> CreateConstantBuffer(const T &data);
};

class DynamicBufferManager;

class DynamicBuffer : public SubBuffer, public Uncopyable
{
public:

    DynamicBuffer() = default;
    ~DynamicBuffer() override;

    DynamicBuffer(DynamicBuffer &&other) noexcept;
    DynamicBuffer &operator=(DynamicBuffer &&other) noexcept;

    void Swap(DynamicBuffer &other) noexcept;

    template<RtrcStruct T>
    void SetData(const T &data); // For constant buffer
    void SetData(const void *data, size_t size, bool isConstantBuffer);

    RC<Buffer> GetFullBuffer() override;
    size_t     GetSubBufferOffset() const override;
    size_t     GetSubBufferSize() const override;

private:

    friend class DynamicBufferManager;

    bool IsUninitializedConstantBuffer() const;

    RC<Buffer> buffer_;
    size_t     offset_ = 0;
    size_t     size_ = 0;

    DynamicBufferManager *manager_ = nullptr;
    int32_t               chunkIndex_ = 0;
    int                   slabIndex_ = 0; // -1 means unpooled
};

class DynamicBufferManager : public Uncopyable, public BufferManagerInterface, public ConstantBufferManagerInterface
{
public:

    DynamicBufferManager(RHI::DeviceOPtr device, DeviceSynchronizer &sync, size_t chunkSize = 4 * 1024 * 1024);
    ~DynamicBufferManager() override;

    RC<DynamicBuffer> Create();

    RC<SubBuffer> CreateConstantBuffer(const void *data, size_t size) override;

    void _internalSetData(DynamicBuffer &buffer, const void *data, size_t size, bool isConstantBuffer);
    void _internalRelease(DynamicBuffer &buffer);
    void _internalRelease(Buffer &buffer) override;

private:

    static constexpr size_t MIN_SLAB_SIZE = 16;

    struct Chunk
    {
        RC<Buffer> buffer;
        unsigned char *mappedBuffer = nullptr;
    };

    struct FreeBufferRecord
    {
        int32_t chunkIndex = 0;
        uint32_t offsetInChunk = 0;
    };

    struct Slab
    {
        std::vector<FreeBufferRecord> freeRecords;
        std::mutex mutex;
    };

    // Shared by DynamicBufferManager & DeviceSynchronizer
    // To allow accessing slabs in frame-complete callbacks after dynamic buffer manager is destroyed
    struct SharedData
    {
        std::unique_ptr<Slab[]> slabs;
    };

    void ComputeSlabIndex(size_t size, int &slabIndex, size_t &slabSize);

    RHI::DeviceOPtr device_;
    DeviceSynchronizer &sync_;

    size_t chunkSize_;
    size_t cbufferAlignment_;
    size_t cbufferSizeAlignment_;

    SlotVector<Chunk> chunks_;
    std::shared_mutex chunkMutex_;

    RC<SharedData> sharedData_;
};

template<RtrcStruct T, typename F>
decltype(auto) FlattenConstantBufferStruct(const T &data, F &&f)
{
    constexpr size_t deviceSize = 4 * Rtrc::GetDeviceDWordCount<T>();
    if(deviceSize <= 1024)
    {
        unsigned char flattenData[1024];
        Rtrc::ToDeviceLayout<T>(&data, flattenData);
        return std::invoke(std::forward<F>(f), flattenData, deviceSize);
    }
    std::vector<unsigned char> flattenData(deviceSize);
    Rtrc::ToDeviceLayout<T>(&data, flattenData.data());
    return std::invoke(std::forward<F>(f), flattenData.data(), deviceSize);
}

template<RtrcStruct T>
RC<SubBuffer> ConstantBufferManagerInterface::CreateConstantBuffer(const T &data)
{
    return Rtrc::FlattenConstantBufferStruct(data, [this](const void *flattenData, size_t size)
    {
        return this->CreateConstantBuffer(flattenData, size);
    });
}

inline DynamicBuffer::~DynamicBuffer()
{
    if(manager_)
    {
        manager_->_internalRelease(*this);
    }
}

inline void DynamicBuffer::Swap(DynamicBuffer &other) noexcept
{
    RTRC_SWAP_MEMBERS(
        *this, other,
        buffer_, offset_, size_, manager_, chunkIndex_, slabIndex_);
}

template<RtrcStruct T>
void DynamicBuffer::SetData(const T &data)
{
    Rtrc::FlattenConstantBufferStruct(data, [&](const void *flattenData, size_t size)
    {
        this->SetData(flattenData, size, true);
    });
}

inline void DynamicBuffer::SetData(const void *data, size_t size, bool isConstantBuffer)
{
    manager_->_internalSetData(*this, data, size, isConstantBuffer);
}

inline RC<Buffer> DynamicBuffer::GetFullBuffer()
{
    return buffer_;
}

inline size_t DynamicBuffer::GetSubBufferOffset() const
{
    return offset_;
}

inline size_t DynamicBuffer::GetSubBufferSize() const
{
    return size_;
}

inline bool DynamicBuffer::IsUninitializedConstantBuffer() const
{
    return slabIndex_ < 0;
}

inline RC<DynamicBuffer> DynamicBufferManager::Create()
{
    auto ret = MakeRC<DynamicBuffer>();
    ret->manager_ = this;
    return ret;
}

inline RC<SubBuffer> DynamicBufferManager::CreateConstantBuffer(const void *data, size_t size)
{
    auto ret = Create();
    ret->SetData(data, size, true);
    return ret;
}

RTRC_END
