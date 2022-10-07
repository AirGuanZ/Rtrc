#pragma once

#include <Rtrc/Graphics/Object/BatchReleaseHelper.h>

RTRC_BEGIN

class BufferManager;
class CommandBufferManager;

class UnsynchronizedBufferAccess
{
public:

    enum Type
    {
        None,
        Regular
    };

    struct NoneData
    {

    };

    struct RegularData
    {
        RHI::PipelineStageFlag  stages;
        RHI::ResourceAccessFlag accesses;
    };

    static UnsynchronizedBufferAccess CreateNone();
    static UnsynchronizedBufferAccess CreateRegular(
        RHI::PipelineStageFlag  stages,
        RHI::ResourceAccessFlag accesses);

    Type GetType() const;
    
    void SetNone();
    void SetRegular(
        RHI::PipelineStageFlag  stages,
        RHI::ResourceAccessFlag accesses);
    
    const NoneData    &GetNoneData() const;
    const RegularData &GetRegularData() const;

private:

    Type type_ = None;
    Variant<NoneData, RegularData> data_ = NoneData{};
};

class Buffer : public Uncopyable
{
public:

    Buffer() = default;
    ~Buffer();

    Buffer(Buffer &&other) noexcept;
    Buffer &operator=(Buffer &&other) noexcept;

    void Swap(Buffer &other) noexcept;

    size_t GetSize() const;

    const RHI::BufferPtr &GetRHIObject() const;
    operator const RHI::BufferPtr &() const;

    RHI::BufferSRVPtr GetSRV(const RHI::BufferSRVDesc &desc);
    RHI::BufferUAVPtr GetUAV(const RHI::BufferUAVDesc &desc);

          UnsynchronizedBufferAccess &GetUnsyncAccess();
    const UnsynchronizedBufferAccess &GetUnsyncAccess() const;
    void SetUnsyncAccess(const UnsynchronizedBufferAccess &access);

    void Upload(const void *data, size_t offset, size_t size);

private:

    friend class BufferManager;

    BufferManager *manager_ = nullptr;
    size_t size_ = 0;
    RHI::BufferPtr rhiBuffer_;
    UnsynchronizedBufferAccess unsyncAccess_;
};

class BufferManager : public Uncopyable
{
public:

    BufferManager(HostSynchronizer &hostSync, RHI::DevicePtr device);

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
    };

    using BatchReleaser = BatchReleaseHelper<ReleaseRecord>;

    struct ReuseRecord
    {
        int releaseBatchIndex = -1;
        BatchReleaser::DataListIterator releaseRecordIt;
    };

    void ReleaseImpl(Buffer &buf);

    BatchReleaser batchReleaser_;
    RHI::DevicePtr device_;

    std::multimap<RHI::BufferDesc, ReuseRecord> reuseRecords_;
    tbb::spin_mutex reuseRecordsMutex_;

    std::vector<RHI::BufferPtr> garbageBuffers_;
    tbb::spin_mutex garbageMutex_;

    std::list<Buffer> pendingReleaseBuffers_;
    tbb::spin_mutex pendingReleaseBuffersMutex_;
};

inline UnsynchronizedBufferAccess UnsynchronizedBufferAccess::CreateNone()
{
    UnsynchronizedBufferAccess ret;
    ret.SetNone();
    return ret;
}

inline UnsynchronizedBufferAccess UnsynchronizedBufferAccess::CreateRegular(
    RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses)
{
    UnsynchronizedBufferAccess ret;
    ret.SetRegular(stages, accesses);
    return ret;
}

inline UnsynchronizedBufferAccess::Type UnsynchronizedBufferAccess::GetType() const
{
    return type_;
}

inline void UnsynchronizedBufferAccess::SetNone()
{
    type_ = None;
    data_ = NoneData{};
}

inline void UnsynchronizedBufferAccess::SetRegular(
    RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses)
{
    type_ = Regular;
    data_ = RegularData{ stages, accesses };
}

inline const UnsynchronizedBufferAccess::NoneData &UnsynchronizedBufferAccess::GetNoneData() const
{
    assert(type_ == None);
    return data_.As<NoneData>();
}

inline const UnsynchronizedBufferAccess::RegularData &UnsynchronizedBufferAccess::GetRegularData() const
{
    assert(type_ == Regular);
    return data_.As<RegularData>();
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
    rhiBuffer_.Swap(other.rhiBuffer_);
    std::swap(unsyncAccess_, other.unsyncAccess_);
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

inline RHI::BufferSRVPtr Buffer::GetSRV(const RHI::BufferSRVDesc &desc)
{
    return rhiBuffer_->CreateSRV(desc);
}

inline RHI::BufferUAVPtr Buffer::GetUAV(const RHI::BufferUAVDesc &desc)
{
    return rhiBuffer_->CreateUAV(desc);
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
