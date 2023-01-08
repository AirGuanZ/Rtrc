#pragma once

#include <array>

#include <tbb/spin_mutex.h>
#include <tbb/spin_rw_mutex.h>

#include <Rtrc/Graphics/Device/Buffer/Buffer.h>
#include <Rtrc/Graphics/Device/Buffer/ConstantBufferStruct.h>
#include <Rtrc/Utility/SlotVector.h>
#include <Rtrc/Utility/Struct.h>
#include <Rtrc/Utility/Swap.h>

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
    size_t GetSubBufferOffset() const override;
    size_t GetSubBufferSize() const override;

private:

    friend class DynamicBufferManager;

    bool IsUninitializedConstantBuffer() const;

    RC<Buffer> buffer_;
    size_t offset_ = 0;
    size_t size_ = 0;

    DynamicBufferManager *manager_ = nullptr;
    int32_t chunkIndex_ = 0;
    int slabIndex_ = 0; // -1 means unpooled
};

class DynamicBufferManager :
    public Uncopyable,
    public BufferManagerInterface,
    public ConstantBufferManagerInterface
{
public:

    DynamicBufferManager(RHI::DevicePtr device, DeviceSynchronizer &sync, size_t chunkSize = 4 * 1024 * 1024);
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
        tbb::spin_mutex mutex;
    };

    // Shared by DynamicBufferManager & DeviceSynchronizer
    // To allow accessing slabs in frame-complete callbacks after dynamic buffer manager is destroyed
    struct SharedData
    {
        std::unique_ptr<Slab[]> slabs;
    };

    void ComputeSlabIndex(size_t size, int &slabIndex, size_t &slabSize);

    RHI::DevicePtr device_;
    DeviceSynchronizer &sync_;

    size_t chunkSize_;
    size_t cbufferAlignment_;

    SlotVector<Chunk> chunks_;
    tbb::spin_rw_mutex chunkMutex_;

    RC<SharedData> sharedData_;
};

template<RtrcStruct T>
RC<SubBuffer> ConstantBufferManagerInterface::CreateConstantBuffer(const T &data)
{
    constexpr size_t deviceSize = 4 * ConstantBufferDetail::GetConstantBufferDWordCount<T>();
    if constexpr(deviceSize > 2048)
    {
        std::array<uint8_t, deviceSize> flattenData;
        ConstantBufferDetail::ForEachFlattenMember<T>([&]<typename M>(const char *, size_t hostOffset, size_t deviceOffset)
        {
            auto dst = flattenData.data() + deviceOffset;
            auto src = reinterpret_cast<const unsigned char *>(&data) + hostOffset;
            std::memcpy(dst, src, sizeof(M));
        });
        return this->CreateConstantBuffer(flattenData.data(), deviceSize);
    }
    else
    {
        std::vector<uint8_t> flattenData(deviceSize);
        ConstantBufferDetail::ForEachFlattenMember<T>([&]<typename M>(const char *, size_t hostOffset, size_t deviceOffset)
        {
            uint8_t *dst = flattenData.data() + deviceOffset;
            auto src = reinterpret_cast<const unsigned char *>(&data) + hostOffset;
            std::memcpy(dst, src, sizeof(M));
        });
        return this->CreateConstantBuffer(flattenData.data(), deviceSize);
    }
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
        buffer_, offset_, size_, manager_, chunkIndex_, slabIndex_)
}

template<RtrcStruct T>
void DynamicBuffer::SetData(const T &data)
{
    constexpr size_t deviceSize = 4 * ConstantBufferDetail::GetConstantBufferDWordCount<T>();
    if constexpr(deviceSize > 2048)
    {
        // Avoid dynamic allocation for small buffer
        std::array<uint8_t, deviceSize> flattenData;
        ConstantBufferDetail::ForEachFlattenMember<T>([&]<typename M>(const char *, size_t hostOffset, size_t deviceOffset)
        {
            auto dst = flattenData.data() + deviceOffset;
            auto src = reinterpret_cast<const unsigned char *>(&data) + hostOffset;
            std::memcpy(dst, src, sizeof(M));
        });
        this->SetData(flattenData.data(), deviceSize, true);
    }
    else
    {
        std::vector<uint8_t> flattenData(deviceSize);
        ConstantBufferDetail::ForEachFlattenMember<T>([&]<typename M>(const char *, size_t hostOffset, size_t deviceOffset)
        {
            uint8_t *dst = flattenData.data() + deviceOffset;
            auto src = reinterpret_cast<const unsigned char *>(&data) + hostOffset;
            std::memcpy(dst, src, sizeof(M));
        });
        SetData(flattenData.data(), deviceSize, true);
    }
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
