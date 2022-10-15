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
class Texture2D;
class TextureManager;

struct UnsynchronizedTextureAccess
{
    RHI::PipelineStageFlag  stages = RHI::PipelineStage::None;
    RHI::ResourceAccessFlag accesses = RHI::ResourceAccess::None;
    RHI::TextureLayout      layout = RHI::TextureLayout::Undefined;

    static constexpr UnsynchronizedTextureAccess CreateUndefined()
    {
        return { RHI::PipelineStage::None, RHI::ResourceAccess::None, RHI::TextureLayout::Undefined };
    }

    static constexpr UnsynchronizedTextureAccess CreateNone(RHI::TextureLayout layout)
    {
        return { RHI::PipelineStage::None, RHI::ResourceAccess::None, layout };
    }

    static constexpr UnsynchronizedTextureAccess Create(
        RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses, RHI::TextureLayout layout)
    {
        return { stages, accesses, layout };
    }
};

struct TextureSRV
{
    RC<Texture2D> texture;
    RHI::TextureSRVPtr rhiSRV;

    operator const RHI::TextureSRVPtr() const
    {
        return rhiSRV;
    }
};

struct TextureUAV
{
    RC<Texture2D> texture;
    RHI::TextureUAVPtr rhiUAV;

    operator const RHI::TextureUAVPtr() const
    {
        return rhiUAV;
    }
};

struct TextureRTV
{
    RC<Texture2D> texture;
    RHI::TextureRTVPtr rhiRTV;

    operator const RHI::TextureRTVPtr() const
    {
        return rhiRTV;
    }
};

class Texture2D : public Uncopyable, public std::enable_shared_from_this<Texture2D>
{
public:

    static Texture2D FromRHIObject(RHI::TexturePtr rhiTexture);
    ~Texture2D();

    Texture2D(Texture2D &&other) noexcept;
    Texture2D &operator=(Texture2D &&other) noexcept;

    void Swap(Texture2D &other) noexcept;

    // The rhi texture may be reused by other new texture object after this one is destructed
    void AllowReuse(bool allow);

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    Vector2u GetSize() const;

    uint32_t GetArraySize() const;
    uint32_t GetMipLevelCount() const;

    RHI::Viewport GetViewport(float minDepth = 0, float maxDepth = 1) const;
    RHI::Scissor GetScissor() const;

    const RHI::TexturePtr &GetRHIObject() const;
    operator const RHI::TexturePtr&() const;

    TextureSRV GetSRV(const RHI::TextureSRVDesc &desc = {});
    TextureUAV GetUAV(const RHI::TextureUAVDesc &desc = {});
    TextureRTV GetRTV(const RHI::TextureRTVDesc &desc = {});

          UnsynchronizedTextureAccess &GetUnsyncAccess(uint32_t arrayLayer, uint32_t mipLevel);
    const UnsynchronizedTextureAccess &GetUnsyncAccess(uint32_t arrayLayer, uint32_t mipLevel) const;

    void SetUnsyncAccess(uint32_t arrayLayer, uint32_t mipLevel, const UnsynchronizedTextureAccess &access);
    void SetUnsyncAccess(const UnsynchronizedTextureAccess &access);

private:

    friend class TextureManager;

    Texture2D() = default;

    Texture2D(TextureManager *manager, RHI::TexturePtr rhiTexture);

    TextureManager *manager_ = nullptr;

    uint32_t width_           = 0;
    uint32_t height_          = 0;
    uint32_t arrayLayerCount_ = 0;
    uint32_t mipLevelCount_   = 0;

    RHI::TexturePtr rhiTexture_;
    bool allowReuse_ = false;
    TextureSubresourceMap<UnsynchronizedTextureAccess> unsyncAccesses_;
};

class TextureManager : public Uncopyable
{
public:

    TextureManager(HostSynchronizer &hostSync, RHI::DevicePtr device);

    // When allowReuse is true, the returned texture may reuse a previous created rhi texture
    // and has initial unsync access state
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
        bool allowReuse;
    };

    using BatchReleaser = BatchReleaseHelper<ReleaseRecord>;

    struct ReuseRecord
    {
        int releaseBatchIndex = -1;
        BatchReleaser::DataListIterator releaseRecordIt;
    };

    void ReleaseImpl(ReleaseRecord tex);
    
    BatchReleaser batchReleaser_;
    RHI::DevicePtr device_;

    std::multimap<RHI::TextureDesc, ReuseRecord> reuseRecords_;
    tbb::spin_mutex reuseRecordsMutex_;

    std::vector<RHI::TexturePtr> garbageTextures_;
    tbb::spin_mutex garbageMutex_;

    std::list<ReleaseRecord> pendingReleaseTextures_;
    tbb::spin_mutex pendingReleaseTexturesMutex_;
};

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
    std::swap(allowReuse_, other.allowReuse_);
    rhiTexture_.Swap(other.rhiTexture_);
    unsyncAccesses_.Swap(other.unsyncAccesses_);
}

inline void Texture2D::AllowReuse(bool allow)
{
    allowReuse_ = allow;
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

inline RHI::Viewport Texture2D::GetViewport(float minDepth, float maxDepth) const
{
    return RHI::Viewport{
        .lowerLeftCorner = { 0, 0 },
        .size            = { static_cast<float>(width_), static_cast<float>(height_) },
        .minDepth        = minDepth,
        .maxDepth        = maxDepth
    };
}

inline RHI::Scissor Texture2D::GetScissor() const
{
    return RHI::Scissor
    {
        .lowerLeftCorner = { 0, 0 },
        .size = { static_cast<int>(width_), static_cast<int>(height_) }
    };
}

inline const RHI::TexturePtr &Texture2D::GetRHIObject() const
{
    return rhiTexture_;
}

inline Texture2D::operator const ReferenceCountedPtr<RHI::Texture>&() const
{
    return rhiTexture_;
}

inline TextureSRV Texture2D::GetSRV(const RHI::TextureSRVDesc &desc)
{
    return { shared_from_this(), rhiTexture_->CreateSRV(desc) };
}

inline TextureUAV Texture2D::GetUAV(const RHI::TextureUAVDesc &desc)
{
    return { shared_from_this(), rhiTexture_->CreateUAV(desc) };
}

inline TextureRTV Texture2D::GetRTV(const RHI::TextureRTVDesc &desc)
{
    return { shared_from_this(), rhiTexture_->CreateRTV(desc) };
}

inline UnsynchronizedTextureAccess &Texture2D::GetUnsyncAccess(uint32_t arrayLayer, uint32_t mipLevel)
{
    if(unsyncAccesses_.IsEmpty())
    {
        unsyncAccesses_ = TextureSubresourceMap(
            mipLevelCount_, arrayLayerCount_, UnsynchronizedTextureAccess::CreateUndefined());
    }
    return unsyncAccesses_(mipLevel, arrayLayer);
}

inline const UnsynchronizedTextureAccess &Texture2D::GetUnsyncAccess(uint32_t arrayLayer, uint32_t mipLevel) const
{
    if(unsyncAccesses_.IsEmpty())
    {
        static const auto UNKNOWN_RET = UnsynchronizedTextureAccess::CreateUndefined();
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
            mipLevelCount_, arrayLayerCount_, UnsynchronizedTextureAccess::CreateUndefined());
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
