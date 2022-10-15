#pragma once

#include <Rtrc/Graphics/Object/BatchReleaseHelper.h>

RTRC_BEGIN

class Buffer;
class BufferManager;
class CommandBuffer;

struct UnsynchronizedBufferAccess
{
    RHI::PipelineStageFlag  stages = RHI::PipelineStage::None;
    RHI::ResourceAccessFlag accesses = RHI::ResourceAccess::None;

    static UnsynchronizedBufferAccess Create(RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses)
    {
        return { stages, accesses };
    }
};

struct BufferSRV
{
    RC<Buffer> buffer;
    RHI::BufferSRVPtr rhiSRV;
};

struct BufferUAV
{
    RC<Buffer> buffer;
    RHI::BufferUAVPtr rhiUAV;
};

class Buffer : public Uncopyable, public std::enable_shared_from_this<Buffer>
{
public:

    static Buffer FromRHIObject(RHI::BufferPtr rhiBuffer);
    ~Buffer();

    Buffer(Buffer &&other) noexcept;
    Buffer &operator=(Buffer &&other) noexcept;

    void Swap(Buffer &other) noexcept;

    // The rhi buffer may be reused by other new buffer object after this one is destructed
    void AllowReuse(bool allow);

    size_t GetSize() const;

    const RHI::BufferPtr &GetRHIObject() const;
    operator const RHI::BufferPtr &() const;

    BufferSRV GetSRV(const RHI::BufferSRVDesc &desc);
    BufferUAV GetUAV(const RHI::BufferUAVDesc &desc);

          UnsynchronizedBufferAccess &GetUnsyncAccess();
    const UnsynchronizedBufferAccess &GetUnsyncAccess() const;
    void SetUnsyncAccess(const UnsynchronizedBufferAccess &access);

    void Upload(const void *data, size_t offset, size_t size);

private:

    friend class BufferManager;

    Buffer() = default;

    BufferManager *manager_ = nullptr;
    size_t size_ = 0;
    bool allowReuse_ = false;
    RHI::BufferPtr rhiBuffer_;
    UnsynchronizedBufferAccess unsyncAccess_;
};

class BufferManager : public Uncopyable
{
public:

    BufferManager(HostSynchronizer &hostSync, RHI::DevicePtr device);

    // When allowReuse is true, the returned buffer may reuse a previous created rhi buffer
    // and has initial unsync access state
    RC<Buffer> CreateBuffer(
        size_t                    size,
        RHI::BufferUsageFlag      usages,
        RHI::BufferHostAccessType hostAccess,
        bool                      allowReuse);

    void GC();

    void _rtrcReleaseInternal(Buffer &buf);

private:

    struct ReleaseRecord
    {
        RHI::BufferPtr rhiBuffer;
        UnsynchronizedBufferAccess unsyncAccess;
        bool allowReuse;
    };

    using BatchReleaser = BatchReleaseHelper<ReleaseRecord>;

    struct ReuseRecord
    {
        int releaseBatchIndex = -1;
        BatchReleaser::DataListIterator releaseRecordIt;
    };

    void ReleaseImpl(ReleaseRecord buf);

    BatchReleaser batchReleaser_;
    RHI::DevicePtr device_;

    std::multimap<RHI::BufferDesc, ReuseRecord> reuseRecords_;
    tbb::spin_mutex reuseRecordsMutex_;

    std::vector<RHI::BufferPtr> garbageBuffers_;
    tbb::spin_mutex garbageMutex_;

    std::list<ReleaseRecord> pendingReleaseBuffers_;
    tbb::spin_mutex pendingReleaseBuffersMutex_;
};

inline Buffer Buffer::FromRHIObject(RHI::BufferPtr rhiBuffer)
{
    Buffer ret;
    ret.rhiBuffer_ = std::move(rhiBuffer);
    ret.size_ = ret.rhiBuffer_->GetDesc().size;
    return ret;
}

inline Buffer::~Buffer()
{
    if(manager_)
    {
        manager_->_rtrcReleaseInternal(*this);
    }
}

inline Buffer::Buffer(Buffer &&other) noexcept
    : Buffer()
{
    Swap(other);
}

inline Buffer &Buffer::operator=(Buffer &&other) noexcept
{
    Swap(other);
    return *this;
}

inline void Buffer::Swap(Buffer &other) noexcept
{
    std::swap(manager_, other.manager_);
    std::swap(size_, other.size_);
    std::swap(allowReuse_, other.allowReuse_);
    rhiBuffer_.Swap(other.rhiBuffer_);
    std::swap(unsyncAccess_, other.unsyncAccess_);
}

inline void Buffer::AllowReuse(bool allow)
{
    allowReuse_ = allow;
}

inline size_t Buffer::GetSize() const
{
    return size_;
}

inline const RHI::BufferPtr &Buffer::GetRHIObject() const
{
    return rhiBuffer_;
}

inline Buffer::operator const ReferenceCountedPtr<RHI::Buffer>&() const
{
    return rhiBuffer_;
}

inline BufferSRV Buffer::GetSRV(const RHI::BufferSRVDesc &desc)
{
    return { shared_from_this(), rhiBuffer_->CreateSRV(desc)};
}

inline BufferUAV Buffer::GetUAV(const RHI::BufferUAVDesc &desc)
{
    return { shared_from_this(), rhiBuffer_->CreateUAV(desc) };
}

inline UnsynchronizedBufferAccess &Buffer::GetUnsyncAccess()
{
    return unsyncAccess_;
}

inline const UnsynchronizedBufferAccess &Buffer::GetUnsyncAccess() const
{
    return unsyncAccess_;
}

inline void Buffer::SetUnsyncAccess(const UnsynchronizedBufferAccess &access)
{
    unsyncAccess_ = access;
}

RTRC_END
