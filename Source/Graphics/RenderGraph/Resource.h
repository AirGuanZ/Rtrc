#pragma once

#include <Graphics/Device/Buffer.h>
#include <Graphics/Device/Texture.h>
#include <Graphics/RenderGraph/UseInfo.h>

RTRC_RG_BEGIN

class BufferResource;
class RenderGraph;
class TextureResource;

class Resource : public Uncopyable
{
    RenderGraph *graph_;
    int index_;

public:

    Resource(RenderGraph* graph,int resourceIndex) : graph_(graph), index_(resourceIndex) { }

    virtual ~Resource() = default;

    int GetResourceIndex() const { return index_; }

    RenderGraph *GetGraph() const { return graph_; }
};

template<typename T> const T *TryCastResource(const Resource *rsc) { return dynamic_cast<const T *>(rsc); }
template<typename T>       T *TryCastResource(      Resource *rsc) { return dynamic_cast<T *>(rsc); }

class BufferResourceSrv
{
public:

    BufferSrv GetSrv() const;

    RHI::ResourceAccessFlag GetResourceAccess() const;

    const BufferResource *GetResource() const;

private:

    friend class BufferResource;

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

    const BufferResource *resource_ = nullptr;
    Variant<StructuredView, TexelView> desc_;
};

class BufferResourceUav
{
public:

    BufferUav GetUav() const;

    RHI::ResourceAccessFlag GetResourceAccess(bool writeOnly) const;

    const BufferResource *GetResource() const;

private:

    friend class BufferResource;

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

    const BufferResource *resource_ = nullptr;
    Variant<StructuredView, TexelView> desc_;
};

class BufferResource : public Resource
{
public:

    using Resource::Resource;

    RC<Buffer> Get() const;

    virtual RHI::Format GetDefaultTexelFormat() const = 0;
    virtual size_t GetDefaultStructStride() const = 0;

    BufferResourceSrv GetStructuredSrv() const;
    BufferResourceSrv GetStructuredSrv(size_t structStride) const;
    BufferResourceSrv GetStructuredSrv(size_t byteOffset, size_t structStride) const;

    BufferResourceSrv GetTexelSrv() const;
    BufferResourceSrv GetTexelSrv(RHI::Format texelFormat) const;
    BufferResourceSrv GetTexelSrv(size_t byteOffset, RHI::Format texelFormat) const;

    BufferResourceUav GetStructuredUav() const;
    BufferResourceUav GetStructuredUav(size_t structStride) const;
    BufferResourceUav GetStructuredUav(size_t byteOffset, size_t structStride) const;

    BufferResourceUav GetTexelUav() const;
    BufferResourceUav GetTexelUav(RHI::Format texelFormat) const;
    BufferResourceUav GetTexelUav(size_t byteOffset, RHI::Format texelFormat) const;

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

    BufferResourceSrv GetSrvImpl(size_t stride, size_t offset) const;
    BufferResourceSrv GetSrvImpl(RHI::Format format, size_t offset) const;
    BufferResourceUav GetUavImpl(size_t stride, size_t offset) const;
    BufferResourceUav GetUavImpl(RHI::Format format, size_t offset) const;
};

class TlasResource
{
    friend class RenderGraph;

    RC<Tlas>        tlas_;
    BufferResource *tlasBuffer_;

    TlasResource(RC<Tlas> tlas, BufferResource *tlasBuffer): tlas_(std::move(tlas)), tlasBuffer_(tlasBuffer) { }

public:

    BufferResource *GetInternalBuffer() const { return tlasBuffer_; }

    const RC<Tlas> &Get() const;
};

class TextureResourceSrv
{
public:

    TextureSrv GetSrv() const;

    // f(textureSubresource, layout, access)
    template<typename F>
    void ForEachSubresourceAccess(const F &f) const;

    const TextureResource *GetResource() const;

private:

    friend class TextureResource;

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

    const TextureResource *resource_ = nullptr;
    Variant<NonArrayView, ArrayView> desc_;
};

class TextureResourceUav
{
public:

    TextureUav GetUav() const;

    // f(textureSubresource, layout, access)
    template<typename F>
    void ForEachSubresourceAccess(const F &f, bool writeOnly = false) const;

    const TextureResource *GetResource() const;

private:

    friend class TextureResource;

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

    const TextureResource *resource_ = nullptr;
    Variant<NonArrayView, ArrayView> desc_;
};

class TextureResourceRtv
{
public:

    TextureRtv GetRtv() const;

private:

    friend class TextureResource;

    const TextureResource *resource_ = nullptr;
    uint32_t mipLevel_ = 0;
    uint32_t arrayLayer_ = 0;
};

class TextureResourceDsv
{
public:

    TextureDsv GetDsv() const;

private:

    friend class TextureResource;

    const TextureResource *resource_ = nullptr;
    uint32_t mipLevel_ = 0;
    uint32_t arrayLayer_ = 0;
};

class TextureResource : public Resource
{
public:

    using Resource::Resource;

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
    TextureResourceSrv GetSrv() const;
    // non-array view
    TextureResourceSrv GetSrv(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer) const;
    // array view
    TextureResourceSrv GetSrv(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, uint32_t layerCount) const;

    // non-array view for single-layer texture, array view for multi-layer texture
    TextureResourceUav GetUav() const;
    // non-array view
    TextureResourceUav GetUav(uint32_t mipLevel, uint32_t arrayLayer) const;
    // array view
    TextureResourceUav GetUav(uint32_t mipLevel, uint32_t arrayLayer, uint32_t layerCount) const;

    TextureResourceRtv GetRtv(uint32_t mipLevel = 0, uint32_t arrayLayer = 0) const;
    TextureResourceDsv GetDsv(uint32_t mipLevel = 0, uint32_t arrayLayer = 0) const;

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

inline BufferSrv BufferResourceSrv::GetSrv() const
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

inline RHI::ResourceAccessFlag BufferResourceSrv::GetResourceAccess() const
{
    return desc_.Match(
        [&](const StructuredView &v) { return RHI::ResourceAccess::StructuredBufferRead; },
        [&](const TexelView &v) { return RHI::ResourceAccess::BufferRead; });
}

inline const BufferResource *BufferResourceSrv::GetResource() const
{
    return resource_;
}

inline BufferUav BufferResourceUav::GetUav() const
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

inline RHI::ResourceAccessFlag BufferResourceUav::GetResourceAccess(bool writeOnly) const
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

inline const BufferResource *BufferResourceUav::GetResource() const
{
    return resource_;
}

template<typename F>
void TextureResourceSrv::ForEachSubresourceAccess(const F &f) const
{
    auto fimpl = [&](int m, int a)
        {
            const RHI::TextureSubresource subrsc
            {
                .mipLevel = static_cast<uint32_t>(m),
                .arrayLayer = static_cast<uint32_t>(a)
            };
            f(subrsc, RHI::TextureLayout::ShaderTexture, RHI::ResourceAccess::TextureRead);
        };
    desc_.Match(
        [&](const NonArrayView &v)
        {
            for(int mi = 0; mi < v.levelCount; ++mi)
            {
                fimpl(v.mipLevel + mi, v.arrayLayer);
            }
        },
        [&](const ArrayView &v)
        {
            for(int mi = 0; mi < v.levelCount; ++mi)
            {
                for(int ai = 0; ai < v.layerCount; ++ai)
                {
                    fimpl(v.mipLevel + mi, v.arrayLayer + ai);
                }
            }
        });
}

template<typename F>
void TextureResourceUav::ForEachSubresourceAccess(const F &f, bool writeOnly) const
{
    auto fimpl = [&](int m, int a)
        {
            const RHI::TextureSubresource subrsc
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
            for(int ai = 0; ai < v.layerCount; ++ai)
            {
                fimpl(v.mipLevel, v.arrayLayer + ai);
            }
        });
}

inline BufferResourceSrv BufferResource::GetStructuredSrv() const
{
    return GetSrvImpl(GetDefaultStructStride(), 0);
}

inline BufferResourceSrv BufferResource::GetStructuredSrv(size_t structStride) const
{
    return GetSrvImpl(structStride, 0);
}

inline BufferResourceSrv BufferResource::GetStructuredSrv(size_t byteOffset, size_t structStride) const
{
    return GetSrvImpl(structStride, byteOffset);
}

inline BufferResourceSrv BufferResource::GetTexelSrv() const
{
    return GetSrvImpl(GetDefaultTexelFormat(), 0);
}

inline BufferResourceSrv BufferResource::GetTexelSrv(RHI::Format texelFormat) const
{
    return GetSrvImpl(texelFormat, 0);
}

inline BufferResourceSrv BufferResource::GetTexelSrv(size_t byteOffset, RHI::Format texelFormat) const
{
    return GetSrvImpl(texelFormat, byteOffset);
}

inline BufferResourceUav BufferResource::GetStructuredUav() const
{
    return GetUavImpl(GetDefaultStructStride(), 0);
}

inline BufferResourceUav BufferResource::GetStructuredUav(size_t structStride) const
{
    return GetUavImpl(structStride, 0);
}

inline BufferResourceUav BufferResource::GetStructuredUav(size_t byteOffset, size_t structStride) const
{
    return GetUavImpl(structStride, byteOffset);
}

inline BufferResourceUav BufferResource::GetTexelUav() const
{
    return GetUavImpl(GetDefaultTexelFormat(), 0);
}

inline BufferResourceUav BufferResource::GetTexelUav(RHI::Format texelFormat) const
{
    return GetUavImpl(texelFormat, 0);
}

inline BufferResourceUav BufferResource::GetTexelUav(size_t byteOffset, RHI::Format texelFormat) const
{
    return GetUavImpl(texelFormat, byteOffset);
}

inline BufferResourceSrv BufferResource::GetSrvImpl(size_t stride, size_t offset) const
{
    BufferResourceSrv ret;
    ret.resource_ = this;
    ret.desc_ = BufferResourceSrv::StructuredView{ stride, offset };
    return ret;
}

inline BufferResourceSrv BufferResource::GetSrvImpl(RHI::Format format, size_t offset) const
{
    BufferResourceSrv ret;
    ret.resource_ = this;
    ret.desc_ = BufferResourceSrv::TexelView{ format, offset };
    return ret;
}

inline BufferResourceUav BufferResource::GetUavImpl(size_t stride, size_t offset) const
{
    BufferResourceUav ret;
    ret.resource_ = this;
    ret.desc_ = BufferResourceUav::StructuredView{ stride, offset };
    return ret;
}

inline BufferResourceUav BufferResource::GetUavImpl(RHI::Format format, size_t offset) const
{
    BufferResourceUav ret;
    ret.resource_ = this;
    ret.desc_ = BufferResourceUav::TexelView{ format, offset };
    return ret;
}

inline BufferSrv BufferResource::GetStructuredSrvImm() const
{
    return Get()->GetStructuredSrv();
}

inline BufferSrv BufferResource::GetStructuredSrvImm(size_t structStride) const
{
    return Get()->GetStructuredSrv(structStride);
}

inline BufferSrv BufferResource::GetStructuredSrvImm(size_t byteOffset, size_t structStride) const
{
    return Get()->GetStructuredSrv(byteOffset, structStride);
}

inline BufferSrv BufferResource::GetTexelSrvImm() const
{
    return Get()->GetTexelSrv();
}

inline BufferSrv BufferResource::GetTexelSrvImm(RHI::Format texelFormat) const
{
    return Get()->GetTexelSrv(texelFormat);
}

inline BufferSrv BufferResource::GetTexelSrvImm(size_t byteOffset, RHI::Format texelFormat) const
{
    return Get()->GetTexelSrv(byteOffset, texelFormat);
}

inline BufferUav BufferResource::GetStructuredUavImm() const
{
    return Get()->GetStructuredUav();
}

inline BufferUav BufferResource::GetStructuredUavImm(size_t structStride) const
{
    return Get()->GetStructuredUav(structStride);
}

inline BufferUav BufferResource::GetStructuredUavImm(size_t byteOffset, size_t structStride) const
{
    return Get()->GetStructuredUav(byteOffset, structStride);
}

inline BufferUav BufferResource::GetTexelUavImm() const
{
    return Get()->GetTexelUav();
}

inline BufferUav BufferResource::GetTexelUavImm(RHI::Format texelFormat) const
{
    return Get()->GetTexelUav(texelFormat);
}

inline BufferUav BufferResource::GetTexelUavImm(size_t byteOffset, RHI::Format texelFormat) const
{
    return Get()->GetTexelUav(byteOffset, texelFormat);
}

inline TextureSrv TextureResource::GetSrvImm() const
{
    return Get()->GetSrv();
}

inline TextureSrv TextureResource::GetSrvImm(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer) const
{
    return Get()->GetSrv(mipLevel, levelCount, arrayLayer);
}

inline TextureSrv TextureResource::GetSrvImm(
    uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, uint32_t layerCount) const
{
    return Get()->GetSrv(mipLevel, levelCount, arrayLayer, layerCount);
}

inline TextureUav TextureResource::GetUavImm() const
{
    return Get()->GetUav();
}

inline TextureUav TextureResource::GetUavImm(uint32_t mipLevel, uint32_t arrayLayer) const
{
    return Get()->GetUav(mipLevel, arrayLayer);
}

inline TextureUav TextureResource::GetUavImm(uint32_t mipLevel, uint32_t arrayLayer, uint32_t layerCount) const
{
    return Get()->GetUav(mipLevel, arrayLayer, layerCount);
}

inline TextureDsv TextureResource::GetDsvImm(uint32_t mipLevel, uint32_t arrayLayer) const
{
    return Get()->GetDsv(mipLevel, arrayLayer);
}

inline TextureRtv TextureResource::GetRtvImm(uint32_t mipLevel, uint32_t arrayLayer) const
{
    return Get()->GetRtv(mipLevel, arrayLayer);
}

RTRC_RG_END
