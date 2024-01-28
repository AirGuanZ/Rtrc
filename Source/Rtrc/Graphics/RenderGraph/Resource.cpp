#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_RG_BEGIN

RC<Buffer> BufferResource::Get() const
{
    return GetCurrentPassContext().Get(this);
}

const RC<Tlas> &TlasResource::Get() const
{
    assert(GetCurrentPassContext().Get(tlasBuffer_));
    return tlas_;
}

TextureSrv TextureResourceSrv::GetSrv() const
{
    return desc_.Match(
        [&](const NonArrayView &v)
        {
            return resource_->GetSrvImm(v.mipLevel, v.levelCount, v.arrayLayer);
        },
        [&](const ArrayView &v)
        {
            return resource_->GetSrvImm(v.mipLevel, v.levelCount, v.arrayLayer, v.layerCount);
        });
}

TextureResource *TextureResourceSrv::GetResource() const
{
    return resource_;
}

TextureUav TextureResourceUav::GetUav() const
{
    return desc_.Match(
        [&](const NonArrayView &v)
        {
            return resource_->GetUavImm(v.mipLevel, v.arrayLayer);
        },
        [&](const ArrayView &v)
        {
            return resource_->GetUavImm(v.mipLevel, v.arrayLayer, v.layerCount);
        });
}

TextureResource *TextureResourceUav::GetResource() const
{
    return resource_;
}

TextureRtv TextureResourceRtv::GetRtv() const
{
    return resource_->GetRtvImm(mipLevel_, arrayLayer_);
}

TextureDsv TextureResourceDsv::GetDsv() const
{
    return resource_->GetDsvImm(mipLevel_, arrayLayer_);
}

RHI::Viewport TextureResource::GetViewport(float minDepth, float maxDepth) const
{
    return RHI::Viewport::Create(GetDesc(), minDepth, maxDepth);
}

RHI::Scissor TextureResource::GetScissor() const
{
    return RHI::Scissor::Create(GetDesc());
}

RC<Texture> TextureResource::Get() const
{
    return GetCurrentPassContext().Get(this);
}

TextureResourceSrv TextureResource::GetSrv()
{
    auto &desc = GetDesc();
    if(desc.arraySize > 1)
    {
        return GetSrv(0, desc.mipLevels, 0, desc.arraySize);
    }
    return GetSrv(0, desc.mipLevels, 0);
}

TextureResourceSrv TextureResource::GetSrv(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer)
{
    TextureResourceSrv ret;
    ret.resource_ = this;
    ret.desc_ = TextureResourceSrv::NonArrayView{ mipLevel, levelCount, arrayLayer };
    return ret;
}

TextureResourceSrv TextureResource::GetSrv(
    uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, uint32_t layerCount)
{
    TextureResourceSrv ret;
    ret.resource_ = this;
    ret.desc_ = TextureResourceSrv::ArrayView{ mipLevel, levelCount, arrayLayer, layerCount };
    return ret;
}

TextureResourceUav TextureResource::GetUav()
{
    auto &desc = GetDesc();
    if(desc.arraySize > 1)
    {
        return GetUav(0, 0, desc.arraySize);
    }
    return GetUav(0, 0);
}

TextureResourceUav TextureResource::GetUav(uint32_t mipLevel, uint32_t arrayLayer)
{
    TextureResourceUav ret;
    ret.resource_ = this;
    ret.desc_ = TextureResourceUav::NonArrayView{ mipLevel, arrayLayer };
    return ret;
}

TextureResourceUav TextureResource::GetUav(uint32_t mipLevel, uint32_t arrayLayer, uint32_t layerCount)
{
    TextureResourceUav ret;
    ret.resource_ = this;
    ret.desc_ = TextureResourceUav::ArrayView{ mipLevel, arrayLayer, layerCount };
    return ret;
}

TextureResourceRtv TextureResource::GetRtv(uint32_t mipLevel, uint32_t arrayLayer)
{
    TextureResourceRtv ret;
    ret.resource_ = this;
    ret.mipLevel_ = mipLevel;
    ret.arrayLayer_ = arrayLayer;
    return ret;
}

TextureResourceDsv TextureResource::GetDsv(uint32_t mipLevel, uint32_t arrayLayer)
{
    TextureResourceDsv ret;
    ret.resource_ = this;
    ret.mipLevel_ = mipLevel;
    ret.arrayLayer_ = arrayLayer;
    return ret;
}

RTRC_RG_END
