#pragma once

#include <Rtrc/Graphics/Device/AccelerationStructure.h>
#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/Texture.h>
#include <Rtrc/Graphics/RenderGraph/UseInfo.h>

RTRC_BEGIN

class RenderGraph;

class RGBufImpl;
class RGTexImpl;
class RGTlasImpl;

using RGBuffer = RGBufImpl *;
using RGTexture = RGTexImpl *;
using RGTlas = RGTlasImpl *;

class RGResource : public Uncopyable
{
    RenderGraph *graph_;
    int index_;

public:

    RGResource(RenderGraph* graph,int resourceIndex) : graph_(graph), index_(resourceIndex) { }

    virtual ~RGResource() = default;

    int GetResourceIndex() const { return index_; }

    RenderGraph *GetGraph() const { return graph_; }
};

template<typename T> const T *TryCastResource(const RGResource *rsc) { return dynamic_cast<const T *>(rsc); }
template<typename T>       T *TryCastResource(      RGResource *rsc) { return dynamic_cast<T *>(rsc); }

class RGBufSrv
{
public:

    BufferSrv GetSrv() const;

    RHI::ResourceAccessFlag GetResourceAccess() const;

    RGBuffer GetResource() const;

private:

    friend class RGBufImpl;

    struct StructuredView
    {
        size_t byteStride = 0;
        size_t byteOffset = 0;
    };

    struct TexelView
    {
        RHI::Format format = RHI::Format::Unknown;
        size_t byteOffset = 0;
    };

    RGBufImpl *resource_ = nullptr;
    Variant<StructuredView, TexelView> desc_;
};

class RGBufUav
{
public:

    BufferUav GetUav() const;

    RHI::ResourceAccessFlag GetResourceAccess(bool writeOnly) const;

    RGBuffer GetResource() const;

private:

    friend class RGBufImpl;

    struct StructuredView
    {
        size_t byteStride = 0;
        size_t byteOffset = 0;
    };

    struct TexelView
    {
        RHI::Format format = RHI::Format::Unknown;
        size_t byteOffset = 0;
    };

    RGBufImpl *resource_ = nullptr;
    Variant<StructuredView, TexelView> desc_;
};

class RGBufImpl : public RGResource
{
public:

    using RGResource::RGResource;

    virtual const RHI::BufferDesc& GetDesc() const = 0;
    size_t GetSize() const { return this->GetDesc().size; }

    RC<Buffer> Get() const;

    virtual RHI::Format GetDefaultTexelFormat() const = 0;
    virtual size_t GetDefaultStructStride() const = 0;

    virtual void SetDefaultTexelFormat(RHI::Format format) = 0;
    virtual void SetDefaultStructStride(size_t stride) = 0;

    RGBufSrv GetStructuredSrv();
    RGBufSrv GetStructuredSrv(size_t structStride);
    RGBufSrv GetStructuredSrv(size_t byteOffset, size_t structStride);

    RGBufSrv GetTexelSrv();
    RGBufSrv GetTexelSrv(RHI::Format texelFormat);
    RGBufSrv GetTexelSrv(size_t byteOffset, RHI::Format texelFormat);

    RGBufUav GetStructuredUav();
    RGBufUav GetStructuredUav(size_t structStride);
    RGBufUav GetStructuredUav(size_t byteOffset, size_t structStride);

    RGBufUav GetTexelUav();
    RGBufUav GetTexelUav(RHI::Format texelFormat);
    RGBufUav GetTexelUav(size_t byteOffset, RHI::Format texelFormat);

    // Helper methods available when executing pass callback function:

    BufferSrv GetStructuredSrvImm() const;
    BufferSrv GetStructuredSrvImm(size_t structStride) const;
    BufferSrv GetStructuredSrvImm(size_t byteOffset, size_t structStride) const;

    BufferSrv GetTexelSrvImm() const;
    BufferSrv GetTexelSrvImm(RHI::Format texelFormat) const;
    BufferSrv GetTexelSrvImm(size_t byteOffset, RHI::Format texelFormat) const;

    BufferUav GetStructuredUavImm() const;
    BufferUav GetStructuredUavImm(size_t structStride) const;
    BufferUav GetStructuredUavImm(size_t byteOffset, size_t structStride) const;

    BufferUav GetTexelUavImm() const;
    BufferUav GetTexelUavImm(RHI::Format texelFormat) const;
    BufferUav GetTexelUavImm(size_t byteOffset, RHI::Format texelFormat) const;

private:

    RGBufSrv GetSrvImpl(size_t stride, size_t offset);
    RGBufSrv GetSrvImpl(RHI::Format format, size_t offset);
    RGBufUav GetUavImpl(size_t stride, size_t offset);
    RGBufUav GetUavImpl(RHI::Format format, size_t offset);
};

class RGTlasImpl
{
    friend class RenderGraph;

    RC<Tlas> tlas_;
    RGBuffer tlasBuffer_;

    RGTlasImpl(RC<Tlas> tlas, RGBufImpl *tlasBuffer): tlas_(std::move(tlas)), tlasBuffer_(tlasBuffer) { }

public:

    RGBufImpl *GetInternalBuffer() const { return tlasBuffer_; }

    const RC<Tlas> &Get() const;
};

class RGTexSrv
{
public:

    TextureSrv GetSrv() const;

    // f(textureSubresource, layout, access)
    template<typename F>
    void ForEachSubresourceAccess(const F &f) const;

    RGTexture GetResource() const;

    operator bool() const { return resource_ != nullptr; }

private:

    friend class RGTexImpl;

    struct NonArrayView
    {
        uint32_t mipLevel = 0;
        uint32_t levelCount = 0;
        uint32_t arrayLayer = 0;
    };

    struct ArrayView
    {
        uint32_t mipLevel = 0;
        uint32_t levelCount = 0;
        uint32_t arrayLayer = 0;
        uint32_t layerCount = 0;
    };

    RGTexImpl *resource_ = nullptr;
    Variant<NonArrayView, ArrayView> desc_;
};

class RGTexUav
{
public:

    TextureUav GetUav() const;

    // f(textureSubresource, layout, access)
    template<typename F>
    void ForEachSubresourceAccess(const F &f, bool writeOnly = false) const;

    RGTexture GetResource() const;

    operator bool() const { return resource_ != nullptr; }

private:

    friend class RGTexImpl;

    struct NonArrayView
    {
        uint32_t mipLevel = 0;
        uint32_t arrayLayer = 0;
    };

    struct ArrayView
    {
        uint32_t mipLevel = 0;
        uint32_t arrayLayer = 0;
        uint32_t layerCount = 0;
    };

    RGTexImpl *resource_ = nullptr;
    Variant<NonArrayView, ArrayView> desc_;
};

class RGTexRtv
{
public:

    TextureRtv GetRtv() const;

    RGTexture GetResource() const;

    operator bool() const { return resource_ != nullptr; }

private:

    friend class RGTexImpl;

    RGTexImpl *resource_ = nullptr;
    uint32_t mipLevel_ = 0;
    uint32_t arrayLayer_ = 0;
};

class RGTexDsv
{
public:

    TextureDsv GetDsv() const;

    RGTexture GetResource() const;

    operator bool() const { return resource_ != nullptr; }

private:

    friend class RGTexImpl;

    RGTexImpl *resource_ = nullptr;
    uint32_t mipLevel_ = 0;
    uint32_t arrayLayer_ = 0;
};

class RGTexImpl : public RGResource
{
public:

    using RGResource::RGResource;

    virtual const RHI::TextureDesc &GetDesc() const = 0;

    RHI::TextureDimension GetDimension() const { return GetDesc().dim; }
    RHI::Format           GetFormat()    const { return GetDesc().format; }
    uint32_t              GetWidth()     const { return GetDesc().width; }
    uint32_t              GetHeight()    const { return GetDesc().height; }
    Vector2u              GetSize()      const { return { GetWidth(), GetHeight() }; }
    uint32_t              GetDepth()     const { return GetDesc().depth; }
    uint32_t              GetMipLevels() const { return GetDesc().mipLevels; }
    uint32_t              GetArraySize() const { return GetDesc().arraySize; }

    RHI::Viewport GetViewport(float minDepth = 0, float maxDepth = 1) const;
    RHI::Scissor GetScissor() const;

    RC<Texture> Get() const;

    // non-array view for single-layer texture, array view for multi-layer texture
    RGTexSrv GetSrv();
    // non-array view
    RGTexSrv GetSrv(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer);
    // array view
    RGTexSrv GetSrv(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, uint32_t layerCount);

    // non-array view for single-layer texture, array view for multi-layer texture
    RGTexUav GetUav();
    // non-array view
    RGTexUav GetUav(uint32_t mipLevel, uint32_t arrayLayer);
    // array view
    RGTexUav GetUav(uint32_t mipLevel, uint32_t arrayLayer, uint32_t layerCount);

    RGTexRtv GetRtv(uint32_t mipLevel = 0, uint32_t arrayLayer = 0);
    RGTexDsv GetDsv(uint32_t mipLevel = 0, uint32_t arrayLayer = 0);

    // ========= Helper methods available when executing pass callback function =========

    // non-array view for single-layer texture, array view for multi-layer texture
    TextureSrv GetSrvImm() const;
    // non-array view
    TextureSrv GetSrvImm(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer) const;
    // array view
    TextureSrv GetSrvImm(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, uint32_t layerCount) const;

    // non-array view for single-layer texture, array view for multi-layer texture
    TextureUav GetUavImm() const;
    // non-array view
    TextureUav GetUavImm(uint32_t mipLevel, uint32_t arrayLayer) const;
    // array view
    TextureUav GetUavImm(uint32_t mipLevel, uint32_t arrayLayer, uint32_t layerCount) const;

    TextureRtv GetRtvImm(uint32_t mipLevel = 0, uint32_t arrayLayer = 0) const;
    TextureDsv GetDsvImm(uint32_t mipLevel = 0, uint32_t arrayLayer = 0) const;
};

inline BufferSrv RGBufSrv::GetSrv() const
{
    return desc_.Match(
        [&](const StructuredView &v)
        {
            return resource_->GetStructuredSrvImm(v.byteOffset, v.byteStride);
        },
        [&](const TexelView &v)
        {
            return resource_->GetTexelSrvImm(v.byteOffset, v.format);
        });
}

inline RHI::ResourceAccessFlag RGBufSrv::GetResourceAccess() const
{
    return desc_.Match(
        [&](const StructuredView &v) { return RHI::ResourceAccess::StructuredBufferRead; },
        [&](const TexelView &v) { return RHI::ResourceAccess::BufferRead; });
}

inline RGBufImpl *RGBufSrv::GetResource() const
{
    return resource_;
}

inline BufferUav RGBufUav::GetUav() const
{
    return desc_.Match(
        [&](const StructuredView &v)
        {
            return resource_->GetStructuredUavImm(v.byteOffset, v.byteStride);
        },
        [&](const TexelView &v)
        {
            return resource_->GetTexelUavImm(v.byteOffset, v.format);
        });
}

inline RHI::ResourceAccessFlag RGBufUav::GetResourceAccess(bool writeOnly) const
{
    return desc_.Match(
        [&](const StructuredView &v)
        {
            return (writeOnly ? RHI::ResourceAccess::None : RHI::ResourceAccess::RWStructuredBufferRead) |
                   RHI::ResourceAccess::RWStructuredBufferWrite;
        },
        [&](const TexelView &v)
        {
            return (writeOnly ? RHI::ResourceAccess::None : RHI::ResourceAccess::RWBufferRead) |
                   RHI::ResourceAccess::RWBufferWrite;
        });
}

inline RGBufImpl *RGBufUav::GetResource() const
{
    return resource_;
}

template<typename F>
void RGTexSrv::ForEachSubresourceAccess(const F &f) const
{
    auto fimpl = [&](int m, int a)
        {
            const TexSubrsc subrsc
            {
                .mipLevel = static_cast<uint32_t>(m),
                .arrayLayer = static_cast<uint32_t>(a)
            };
            f(subrsc, RHI::TextureLayout::ShaderTexture, RHI::ResourceAccess::TextureRead);
        };
    desc_.Match(
        [&](const NonArrayView &v)
        {
            for(uint32_t mi = 0; mi < v.levelCount; ++mi)
            {
                fimpl(v.mipLevel + mi, v.arrayLayer);
            }
        },
        [&](const ArrayView &v)
        {
            for(uint32_t mi = 0; mi < v.levelCount; ++mi)
            {
                for(uint32_t ai = 0; ai < v.layerCount; ++ai)
                {
                    fimpl(v.mipLevel + mi, v.arrayLayer + ai);
                }
            }
        });
}

template<typename F>
void RGTexUav::ForEachSubresourceAccess(const F &f, bool writeOnly) const
{
    auto fimpl = [&](int m, int a)
        {
            const TexSubrsc subrsc
            {
                .mipLevel = static_cast<uint32_t>(m),
                .arrayLayer = static_cast<uint32_t>(a)
            };
            f(
                subrsc,
                RHI::TextureLayout::ShaderRWTexture,
                RHI::ResourceAccess::RWBufferWrite |
                (writeOnly ? RHI::ResourceAccess::None :
                             RHI::ResourceAccess::RWTextureRead));
        };
    desc_.Match(
        [&](const NonArrayView &v)
        {
            fimpl(v.mipLevel, v.arrayLayer);
        },
        [&](const ArrayView &v)
        {
            for(uint32_t ai = 0; ai < v.layerCount; ++ai)
            {
                fimpl(v.mipLevel, v.arrayLayer + ai);
            }
        });
}

inline RGBufSrv RGBufImpl::GetStructuredSrv()
{
    return GetSrvImpl(GetDefaultStructStride(), 0);
}

inline RGBufSrv RGBufImpl::GetStructuredSrv(size_t structStride)
{
    return GetSrvImpl(structStride, 0);
}

inline RGBufSrv RGBufImpl::GetStructuredSrv(size_t byteOffset, size_t structStride)
{
    return GetSrvImpl(structStride, byteOffset);
}

inline RGBufSrv RGBufImpl::GetTexelSrv()
{
    return GetSrvImpl(GetDefaultTexelFormat(), 0);
}

inline RGBufSrv RGBufImpl::GetTexelSrv(RHI::Format texelFormat)
{
    return GetSrvImpl(texelFormat, 0);
}

inline RGBufSrv RGBufImpl::GetTexelSrv(size_t byteOffset, RHI::Format texelFormat)
{
    return GetSrvImpl(texelFormat, byteOffset);
}

inline RGBufUav RGBufImpl::GetStructuredUav()
{
    return GetUavImpl(GetDefaultStructStride(), 0);
}

inline RGBufUav RGBufImpl::GetStructuredUav(size_t structStride)
{
    return GetUavImpl(structStride, 0);
}

inline RGBufUav RGBufImpl::GetStructuredUav(size_t byteOffset, size_t structStride)
{
    return GetUavImpl(structStride, byteOffset);
}

inline RGBufUav RGBufImpl::GetTexelUav()
{
    return GetUavImpl(GetDefaultTexelFormat(), 0);
}

inline RGBufUav RGBufImpl::GetTexelUav(RHI::Format texelFormat)
{
    return GetUavImpl(texelFormat, 0);
}

inline RGBufUav RGBufImpl::GetTexelUav(size_t byteOffset, RHI::Format texelFormat)
{
    return GetUavImpl(texelFormat, byteOffset);
}

inline RGBufSrv RGBufImpl::GetSrvImpl(size_t stride, size_t offset)
{
    RGBufSrv ret;
    ret.resource_ = this;
    ret.desc_ = RGBufSrv::StructuredView{ stride, offset };
    return ret;
}

inline RGBufSrv RGBufImpl::GetSrvImpl(RHI::Format format, size_t offset)
{
    RGBufSrv ret;
    ret.resource_ = this;
    ret.desc_ = RGBufSrv::TexelView{ format, offset };
    return ret;
}

inline RGBufUav RGBufImpl::GetUavImpl(size_t stride, size_t offset)
{
    RGBufUav ret;
    ret.resource_ = this;
    ret.desc_ = RGBufUav::StructuredView{ stride, offset };
    return ret;
}

inline RGBufUav RGBufImpl::GetUavImpl(RHI::Format format, size_t offset)
{
    RGBufUav ret;
    ret.resource_ = this;
    ret.desc_ = RGBufUav::TexelView{ format, offset };
    return ret;
}

inline BufferSrv RGBufImpl::GetStructuredSrvImm() const
{
    return Get()->GetStructuredSrv();
}

inline BufferSrv RGBufImpl::GetStructuredSrvImm(size_t structStride) const
{
    return Get()->GetStructuredSrv(structStride);
}

inline BufferSrv RGBufImpl::GetStructuredSrvImm(size_t byteOffset, size_t structStride) const
{
    return Get()->GetStructuredSrv(byteOffset, structStride);
}

inline BufferSrv RGBufImpl::GetTexelSrvImm() const
{
    return Get()->GetTexelSrv();
}

inline BufferSrv RGBufImpl::GetTexelSrvImm(RHI::Format texelFormat) const
{
    return Get()->GetTexelSrv(texelFormat);
}

inline BufferSrv RGBufImpl::GetTexelSrvImm(size_t byteOffset, RHI::Format texelFormat) const
{
    return Get()->GetTexelSrv(byteOffset, texelFormat);
}

inline BufferUav RGBufImpl::GetStructuredUavImm() const
{
    return Get()->GetStructuredUav();
}

inline BufferUav RGBufImpl::GetStructuredUavImm(size_t structStride) const
{
    return Get()->GetStructuredUav(structStride);
}

inline BufferUav RGBufImpl::GetStructuredUavImm(size_t byteOffset, size_t structStride) const
{
    return Get()->GetStructuredUav(byteOffset, structStride);
}

inline BufferUav RGBufImpl::GetTexelUavImm() const
{
    return Get()->GetTexelUav();
}

inline BufferUav RGBufImpl::GetTexelUavImm(RHI::Format texelFormat) const
{
    return Get()->GetTexelUav(texelFormat);
}

inline BufferUav RGBufImpl::GetTexelUavImm(size_t byteOffset, RHI::Format texelFormat) const
{
    return Get()->GetTexelUav(byteOffset, texelFormat);
}

inline TextureSrv RGTexImpl::GetSrvImm() const
{
    return Get()->GetSrv();
}

inline TextureSrv RGTexImpl::GetSrvImm(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer) const
{
    return Get()->GetSrv(mipLevel, levelCount, arrayLayer);
}

inline TextureSrv RGTexImpl::GetSrvImm(
    uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, uint32_t layerCount) const
{
    return Get()->GetSrv(mipLevel, levelCount, arrayLayer, layerCount);
}

inline TextureUav RGTexImpl::GetUavImm() const
{
    return Get()->GetUav();
}

inline TextureUav RGTexImpl::GetUavImm(uint32_t mipLevel, uint32_t arrayLayer) const
{
    return Get()->GetUav(mipLevel, arrayLayer);
}

inline TextureUav RGTexImpl::GetUavImm(uint32_t mipLevel, uint32_t arrayLayer, uint32_t layerCount) const
{
    return Get()->GetUav(mipLevel, arrayLayer, layerCount);
}

inline TextureDsv RGTexImpl::GetDsvImm(uint32_t mipLevel, uint32_t arrayLayer) const
{
    return Get()->GetDsv(mipLevel, arrayLayer);
}

inline TextureRtv RGTexImpl::GetRtvImm(uint32_t mipLevel, uint32_t arrayLayer) const
{
    return Get()->GetRtv(mipLevel, arrayLayer);
}

RTRC_END
