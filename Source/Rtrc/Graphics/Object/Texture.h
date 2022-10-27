#pragma once

#include <list>
#include <map>

#include <Rtrc/Graphics/Object/BatchReleaseHelper.h>
#include <Rtrc/Graphics/Object/TextureSubresourceMap.h>

RTRC_BEGIN

class CommandBuffer;
class ImageDynamic;
class Texture;
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

    bool IsEmpty() const
    {
        return stages == RHI::PipelineStage::None && accesses == RHI::ResourceAccess::None;
    }
};

struct TextureSRV
{
    RC<Texture> tex;
    RHI::TextureSRVPtr rhiSRV;

    TextureSRV() = default;

    TextureSRV(const RC<Texture> &tex);

    operator const RHI::TextureSRVPtr() const
    {
        return rhiSRV;
    }
};

struct TextureUAV
{
    RC<Texture> tex;
    RHI::TextureUAVPtr rhiUAV;

    TextureUAV() = default;

    TextureUAV(const RC<Texture> &tex);

    operator const RHI::TextureUAVPtr() const
    {
        return rhiUAV;
    }
};

struct TextureRTV
{
    RC<Texture> tex;
    RHI::TextureRTVPtr rhiRTV;

    TextureRTV() = default;

    TextureRTV(const RC<Texture> &tex);

    operator const RHI::TextureRTVPtr() const
    {
        return rhiRTV;
    }

    operator RHI::TextureRTV *() const
    {
        return rhiRTV;
    }
};

class Texture : public Uncopyable, public std::enable_shared_from_this<Texture>
{
public:

    static Texture FromRHIObject(RHI::TexturePtr rhiTexture);
    ~Texture();

    Texture(Texture &&other) noexcept;
    Texture &operator=(Texture &&other) noexcept;

    void Swap(Texture &other) noexcept;

    void SetName(std::string name);

    // The rhi texture may be reused by other new texture object after this one is destructed
    void AllowReuse(bool allow);

    RHI::TextureDimension GetDimension() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    uint32_t GetDepth() const;

    uint32_t GetArraySize() const;
    uint32_t GetMipLevelCount() const;

    RHI::Viewport GetViewport(float minDepth = 0, float maxDepth = 1) const;
    RHI::Scissor GetScissor() const;

    const RHI::TextureDesc &GetRHIObjectDesc() const;
    const RHI::TexturePtr &GetRHIObject() const;
    operator const RHI::TexturePtr&() const;

    TextureSRV GetSRV(const RHI::TextureSRVDesc &desc = {});
    TextureUAV GetUAV(const RHI::TextureUAVDesc &desc = {});
    TextureRTV GetRTV(const RHI::TextureRTVDesc &desc = {});

    operator TextureSRV() { return GetSRV(); }
    operator TextureUAV() { return GetUAV(); }
    operator TextureRTV() { return GetRTV(); }

          UnsynchronizedTextureAccess &GetUnsyncAccess(uint32_t arrayLayer, uint32_t mipLevel);
    const UnsynchronizedTextureAccess &GetUnsyncAccess(uint32_t arrayLayer, uint32_t mipLevel) const;

    void SetUnsyncAccess(uint32_t arrayLayer, uint32_t mipLevel, const UnsynchronizedTextureAccess &access);
    void SetUnsyncAccess(const UnsynchronizedTextureAccess &access);

private:

    friend class TextureManager;

    Texture() = default;

    Texture(TextureManager *manager, RHI::TexturePtr rhiTexture);

    TextureManager *manager_ = nullptr;

    RHI::TextureDimension dim_ = RHI::TextureDimension::Tex2D;

    uint32_t width_         = 1;
    uint32_t height_        = 1;
    uint32_t depth_         = 1;
    uint32_t arraySize_     = 1;
    uint32_t mipLevelCount_ = 1;

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
    RC<Texture> CreateTexture2D(
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

    void _rtrcReleaseInternal(Texture &tex);

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

inline TextureSRV::TextureSRV(const RC<Texture> &tex)
{
    *this = tex->GetSRV();
}

inline TextureUAV::TextureUAV(const RC<Texture> &tex)
{
    *this = tex->GetUAV();
}

inline TextureRTV::TextureRTV(const RC<Texture> &tex)
{
    *this = tex->GetRTV();
}

inline Texture Texture::FromRHIObject(RHI::TexturePtr rhiTexture)
{
    auto &desc = rhiTexture->GetDesc();
    Texture ret;
    ret.rhiTexture_ = std::move(rhiTexture);
    ret.width_ = desc.width;
    ret.height_ = desc.height;
    ret.arraySize_ = desc.arraySize;
    ret.mipLevelCount_ = desc.mipLevels;
    return ret;
}

inline Texture::~Texture()
{
    if(manager_)
    {
        manager_->_rtrcReleaseInternal(*this);
    }
}

inline Texture::Texture(Texture &&other) noexcept
    : Texture()
{
    Swap(other);
}

inline Texture &Texture::operator=(Texture &&other) noexcept
{
    Swap(other);
    return *this;
}

inline void Texture::Swap(Texture &other) noexcept
{
    std::swap(manager_, other.manager_);
    std::swap(dim_, other.dim_);
    std::swap(width_, other.width_);
    std::swap(height_, other.height_);
    std::swap(depth_, other.depth_);
    std::swap(arraySize_, other.arraySize_);
    std::swap(mipLevelCount_, other.mipLevelCount_);
    std::swap(allowReuse_, other.allowReuse_);
    rhiTexture_.Swap(other.rhiTexture_);
    unsyncAccesses_.Swap(other.unsyncAccesses_);
}

inline void Texture::SetName(std::string name)
{
    rhiTexture_->SetName(std::move(name));
}

inline void Texture::AllowReuse(bool allow)
{
    allowReuse_ = allow;
}

inline RHI::TextureDimension Texture::GetDimension() const
{
    return dim_;
}

inline uint32_t Texture::GetWidth() const
{
    return width_;
}

inline uint32_t Texture::GetHeight() const
{
    return height_;
}

inline uint32_t Texture::GetDepth() const
{
    assert(dim_ == RHI::TextureDimension::Tex3D);
    return depth_;
}

inline uint32_t Texture::GetArraySize() const
{
    return arraySize_;
}

inline uint32_t Texture::GetMipLevelCount() const
{
    return mipLevelCount_;
}

inline RHI::Viewport Texture::GetViewport(float minDepth, float maxDepth) const
{
    return RHI::Viewport{
        .lowerLeftCorner = { 0, 0 },
        .size            = { static_cast<float>(width_), static_cast<float>(height_) },
        .minDepth        = minDepth,
        .maxDepth        = maxDepth
    };
}

inline RHI::Scissor Texture::GetScissor() const
{
    return RHI::Scissor
    {
        .lowerLeftCorner = { 0, 0 },
        .size = { static_cast<int>(width_), static_cast<int>(height_) }
    };
}

inline const RHI::TextureDesc &Texture::GetRHIObjectDesc() const
{
    return rhiTexture_->GetDesc();
}

inline const RHI::TexturePtr &Texture::GetRHIObject() const
{
    return rhiTexture_;
}

inline Texture::operator const ReferenceCountedPtr<RHI::Texture>&() const
{
    return rhiTexture_;
}

inline TextureSRV Texture::GetSRV(const RHI::TextureSRVDesc &desc)
{
    TextureSRV ret;
    ret.tex = shared_from_this();
    ret.rhiSRV = rhiTexture_->CreateSRV(desc);
    return ret;
}

inline TextureUAV Texture::GetUAV(const RHI::TextureUAVDesc &desc)
{
    TextureUAV ret;
    ret.tex = shared_from_this();
    ret.rhiUAV = rhiTexture_->CreateUAV(desc);
    return ret;
}

inline TextureRTV Texture::GetRTV(const RHI::TextureRTVDesc &desc)
{
    TextureRTV ret;
    ret.tex = shared_from_this();
    ret.rhiRTV = rhiTexture_->CreateRTV(desc);
    return ret;
}

inline UnsynchronizedTextureAccess &Texture::GetUnsyncAccess(uint32_t arrayLayer, uint32_t mipLevel)
{
    if(unsyncAccesses_.IsEmpty())
    {
        unsyncAccesses_ = TextureSubresourceMap(
            mipLevelCount_, arraySize_, UnsynchronizedTextureAccess::CreateUndefined());
    }
    return unsyncAccesses_(mipLevel, arrayLayer);
}

inline const UnsynchronizedTextureAccess &Texture::GetUnsyncAccess(uint32_t arrayLayer, uint32_t mipLevel) const
{
    if(unsyncAccesses_.IsEmpty())
    {
        static const auto UNKNOWN_RET = UnsynchronizedTextureAccess::CreateUndefined();
        return UNKNOWN_RET;
    }
    return unsyncAccesses_(mipLevel, arrayLayer);
}

inline void Texture::SetUnsyncAccess(
    uint32_t arrayLayer, uint32_t mipLevel, const UnsynchronizedTextureAccess &access)
{
    if(unsyncAccesses_.IsEmpty())
    {
        unsyncAccesses_ = TextureSubresourceMap(
            mipLevelCount_, arraySize_, UnsynchronizedTextureAccess::CreateUndefined());
    }
    unsyncAccesses_(mipLevel, arrayLayer) = access;
}

inline void Texture::SetUnsyncAccess(const UnsynchronizedTextureAccess &access)
{
    if(unsyncAccesses_.IsEmpty())
    {
        unsyncAccesses_ = TextureSubresourceMap(mipLevelCount_, arraySize_, access);
    }
    else
    {
        unsyncAccesses_.Fill(access);
    }
}

RTRC_END
