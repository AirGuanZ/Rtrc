#pragma once

#include <list>
#include <map>

#include <Rtrc/Graphics/Object/BatchReleaseHelper.h>
#include <Rtrc/Graphics/Object/CommandBuffer.h>
#include <Rtrc/Graphics/Resource/TextureSubresourceMap.h>
#include <Rtrc/Utils/SlotVector.h>

RTRC_BEGIN

class CommandBuffer;
class ImageDynamic;
class TextureManager;

class UnsynchronizedTextureAccess
{
public:

    enum Type
    {
        Unknown,
        None,
        Regular
    };

    struct UnknownData
    {
        
    };

    struct NoneData
    {
        RHI::TextureLayout layout;
    };

    struct RegularData
    {
        RHI::PipelineStageFlag  stages;
        RHI::ResourceAccessFlag accesses;
        RHI::TextureLayout      layout;
    };

    static UnsynchronizedTextureAccess CreateUnknown();
    static UnsynchronizedTextureAccess CreateNone(RHI::TextureLayout layout);
    static UnsynchronizedTextureAccess CreateRegular(
        RHI::PipelineStageFlag  stages,
        RHI::ResourceAccessFlag accesses,
        RHI::TextureLayout      layout);

    Type GetType() const;

    void SetUnknown();
    void SetNone(RHI::TextureLayout layout);
    void SetRegular(RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses, RHI::TextureLayout layout);

    const UnknownData &GetUnknownData() const;
    const NoneData    &GetNoneData() const;
    const RegularData &GetRegularData() const;

private:

    Type type_ = Unknown;
    Variant<UnknownData, NoneData, RegularData> data_ = UnknownData{};
};

class Texture2D : public Uncopyable
{
public:

    static Texture2D FromRHIObject(RHI::TexturePtr rhiTexture);

    Texture2D() = default;
    ~Texture2D();

    Texture2D(Texture2D &&other) noexcept;
    Texture2D &operator=(Texture2D &&other) noexcept;

    void Swap(Texture2D &other) noexcept;

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    Vector2u GetSize() const;

    uint32_t GetArraySize() const;
    uint32_t GetMipLevelCount() const;

    const RHI::TexturePtr &GetRHIObject() const;
    operator const RHI::TexturePtr&() const;

    RHI::TextureSRVPtr GetSRV(const RHI::TextureSRVDesc &desc);
    RHI::TextureUAVPtr GetUAV(const RHI::TextureUAVDesc &desc);
    RHI::TextureRTVPtr GetRTV(const RHI::TextureRTVDesc &desc);

          UnsynchronizedTextureAccess &GetUnsyncAccess(uint32_t arrayLayer, uint32_t mipLevel);
    const UnsynchronizedTextureAccess &GetUnsyncAccess(uint32_t arrayLayer, uint32_t mipLevel) const;

    void SetUnsyncAccess(uint32_t arrayLayer, uint32_t mipLevel, const UnsynchronizedTextureAccess &access);
    void SetUnsyncAccess(const UnsynchronizedTextureAccess &access);

private:

    friend class TextureManager;

    Texture2D(TextureManager *manager, RHI::TexturePtr rhiTexture);

    TextureManager *manager_ = nullptr;

    uint32_t width_           = 0;
    uint32_t height_          = 0;
    uint32_t arrayLayerCount_ = 0;
    uint32_t mipLevelCount_   = 0;

    RHI::TexturePtr rhiTexture_;

    TextureSubresourceMap<UnsynchronizedTextureAccess> unsyncAccesses_;
};

class TextureManager : public Uncopyable
{
public:

    TextureManager(HostSynchronizer &hostSync, RHI::DevicePtr device);

    RC<Texture2D> CreateTexture2D(
        uint32_t                       width,
        uint32_t                       height,
        uint32_t                       arraySize,
        uint32_t                       mipLevelCount, // '<= 0' means full mipmap chain
        RHI::Format                    format,
        RHI::TextureUsageFlag          usages,
        RHI::QueueConcurrentAccessMode concurrentMode,
        uint32_t                       sampleCount,
        bool                           allowReuse);

    void GC();

    void _rtrcReleaseInternal(Texture2D &tex);

private:

    struct ReleaseRecord
    {
        RHI::TexturePtr rhiTexture;
        TextureSubresourceMap<UnsynchronizedTextureAccess> unsyncAccesses;
    };

    using BatchReleaser = BatchReleaseHelper<ReleaseRecord>;

    struct ReuseRecord
    {
        int releaseBatchIndex = -1;
        BatchReleaser::DataListIterator releaseRecordIt;
    };

    void ReleaseImpl(Texture2D &tex);
    
    BatchReleaser batchReleaser_;
    RHI::DevicePtr device_;

    std::multimap<RHI::TextureDesc, ReuseRecord> reuseRecords_;
    tbb::spin_mutex reuseRecordsMutex_;

    std::vector<RHI::TexturePtr> garbageTextures_;
    tbb::spin_mutex garbageMutex_;

    std::list<Texture2D> pendingReleaseTextures_;
    tbb::spin_mutex pendingReleaseTexturesMutex_;
};

inline UnsynchronizedTextureAccess UnsynchronizedTextureAccess::CreateUnknown()
{
    UnsynchronizedTextureAccess ret;
    ret.SetUnknown();
    return ret;
}

inline UnsynchronizedTextureAccess UnsynchronizedTextureAccess::CreateNone(RHI::TextureLayout layout)
{
    UnsynchronizedTextureAccess ret;
    ret.SetNone(layout);
    return ret;
}

inline UnsynchronizedTextureAccess UnsynchronizedTextureAccess::CreateRegular(
    RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses, RHI::TextureLayout layout)
{
    UnsynchronizedTextureAccess ret;
    ret.SetRegular(stages, accesses, layout);
    return ret;
}

inline UnsynchronizedTextureAccess::Type UnsynchronizedTextureAccess::GetType() const
{
    return type_;
}

inline void UnsynchronizedTextureAccess::SetUnknown()
{
    type_ = Unknown;
    data_ = UnknownData{};
}

inline void UnsynchronizedTextureAccess::SetNone(RHI::TextureLayout layout)
{
    type_ = None;
    data_ = NoneData{ layout };
}

inline void UnsynchronizedTextureAccess::SetRegular(
    RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses, RHI::TextureLayout layout)
{
    type_ = Regular;
    data_ = RegularData{ stages, accesses, layout };
}

inline const UnsynchronizedTextureAccess::UnknownData &UnsynchronizedTextureAccess::GetUnknownData() const
{
    assert(type_ == Unknown);
    return data_.As<UnknownData>();
}

inline const UnsynchronizedTextureAccess::NoneData &UnsynchronizedTextureAccess::GetNoneData() const
{
    assert(type_ == None);
    return data_.As<NoneData>();
}

inline const UnsynchronizedTextureAccess::RegularData &UnsynchronizedTextureAccess::GetRegularData() const
{
    assert(type_ == Regular);
    return data_.As<RegularData>();
}

inline Texture2D Texture2D::FromRHIObject(RHI::TexturePtr rhiTexture)
{
    auto &desc = rhiTexture->GetDesc();
    Texture2D ret;
    ret.rhiTexture_ = std::move(rhiTexture);
    ret.width_ = desc.width;
    ret.height_ = desc.height;
    ret.arrayLayerCount_ = desc.arraySize;
    ret.mipLevelCount_ = desc.mipLevels;
    return ret;
}

inline Texture2D::~Texture2D()
{
    if(manager_)
    {
        manager_->_rtrcReleaseInternal(*this);
    }
}

inline Texture2D::Texture2D(Texture2D &&other) noexcept
    : Texture2D()
{
    Swap(other);
}

inline Texture2D &Texture2D::operator=(Texture2D &&other) noexcept
{
    Swap(other);
    return *this;
}

inline void Texture2D::Swap(Texture2D &other) noexcept
{
    std::swap(manager_, other.manager_);
    std::swap(width_, other.width_);
    std::swap(height_, other.height_);
    std::swap(arrayLayerCount_, other.arrayLayerCount_);
    std::swap(mipLevelCount_, other.mipLevelCount_);
    rhiTexture_.Swap(other.rhiTexture_);
    unsyncAccesses_.Swap(other.unsyncAccesses_);
}

inline uint32_t Texture2D::GetWidth() const
{
    return width_;
}

inline uint32_t Texture2D::GetHeight() const
{
    return height_;
}

inline Vector2u Texture2D::GetSize() const
{
    return { width_, height_ };
}

inline uint32_t Texture2D::GetArraySize() const
{
    return arrayLayerCount_;
}

inline uint32_t Texture2D::GetMipLevelCount() const
{
    return mipLevelCount_;
}

inline const RHI::TexturePtr &Texture2D::GetRHIObject() const
{
    return rhiTexture_;
}

inline Texture2D::operator const ReferenceCountedPtr<RHI::Texture>&() const
{
    return rhiTexture_;
}

inline RHI::TextureSRVPtr Texture2D::GetSRV(const RHI::TextureSRVDesc &desc)
{
    return rhiTexture_->CreateSRV(desc);
}

inline RHI::TextureUAVPtr Texture2D::GetUAV(const RHI::TextureUAVDesc &desc)
{
    return rhiTexture_->CreateUAV(desc);
}

inline RHI::TextureRTVPtr Texture2D::GetRTV(const RHI::TextureRTVDesc &desc)
{
    return rhiTexture_->CreateRTV(desc);
}

inline UnsynchronizedTextureAccess &Texture2D::GetUnsyncAccess(uint32_t arrayLayer, uint32_t mipLevel)
{
    if(unsyncAccesses_.IsEmpty())
    {
        unsyncAccesses_ = TextureSubresourceMap(
            mipLevelCount_, arrayLayerCount_, UnsynchronizedTextureAccess::CreateUnknown());
    }
    return unsyncAccesses_(mipLevel, arrayLayer);
}

inline const UnsynchronizedTextureAccess &Texture2D::GetUnsyncAccess(uint32_t arrayLayer, uint32_t mipLevel) const
{
    if(unsyncAccesses_.IsEmpty())
    {
        static const auto UNKNOWN_RET = UnsynchronizedTextureAccess::CreateUnknown();
        return UNKNOWN_RET;
    }
    return unsyncAccesses_(mipLevel, arrayLayer);
}

inline void Texture2D::SetUnsyncAccess(
    uint32_t arrayLayer, uint32_t mipLevel, const UnsynchronizedTextureAccess &access)
{
    if(unsyncAccesses_.IsEmpty())
    {
        unsyncAccesses_ = TextureSubresourceMap(
            mipLevelCount_, arrayLayerCount_, UnsynchronizedTextureAccess::CreateUnknown());
    }
    unsyncAccesses_(mipLevel, arrayLayer) = access;
}

inline void Texture2D::SetUnsyncAccess(const UnsynchronizedTextureAccess &access)
{
    if(unsyncAccesses_.IsEmpty())
    {
        unsyncAccesses_ = TextureSubresourceMap(mipLevelCount_, arrayLayerCount_, access);
    }
    else
    {
        unsyncAccesses_.Fill(access);
    }
}

RTRC_END
