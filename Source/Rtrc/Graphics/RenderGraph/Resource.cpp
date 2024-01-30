#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_BEGIN

RC<Buffer> RGBufImpl::Get() const
{
    return RGGetPassContext().Get(this);
}

const RC<Tlas> &RGTlasImpl::Get() const
{
    assert(RGGetPassContext().Get(tlasBuffer_));
    return tlas_;
}

TextureSrv RGTexSrv::GetSrv() const
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

RGTexImpl *RGTexSrv::GetResource() const
{
    return resource_;
}

TextureUav RGTexUav::GetUav() const
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

RGTexImpl *RGTexUav::GetResource() const
{
    return resource_;
}

TextureRtv RGTexRtv::GetRtv() const
{
    return resource_->GetRtvImm(mipLevel_, arrayLayer_);
}

RGTexImpl* RGTexRtv::GetResource() const
{
    return resource_;
}

TextureDsv RGTexDsv::GetDsv() const
{
    return resource_->GetDsvImm(mipLevel_, arrayLayer_);
}

RGTexImpl* RGTexDsv::GetResource() const
{
    return resource_;
}

RHI::Viewport RGTexImpl::GetViewport(float minDepth, float maxDepth) const
{
    return RHI::Viewport::Create(GetDesc(), minDepth, maxDepth);
}

RHI::Scissor RGTexImpl::GetScissor() const
{
    return RHI::Scissor::Create(GetDesc());
}

RC<Texture> RGTexImpl::Get() const
{
    return RGGetPassContext().Get(this);
}

RGTexSrv RGTexImpl::GetSrv()
{
    auto &desc = GetDesc();
    if(desc.arraySize > 1)
    {
        return GetSrv(0, desc.mipLevels, 0, desc.arraySize);
    }
    return GetSrv(0, desc.mipLevels, 0);
}

RGTexSrv RGTexImpl::GetSrv(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer)
{
    RGTexSrv ret;
    ret.resource_ = this;
    ret.desc_ = RGTexSrv::NonArrayView{ mipLevel, levelCount, arrayLayer };
    return ret;
}

RGTexSrv RGTexImpl::GetSrv(
    uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, uint32_t layerCount)
{
    RGTexSrv ret;
    ret.resource_ = this;
    ret.desc_ = RGTexSrv::ArrayView{ mipLevel, levelCount, arrayLayer, layerCount };
    return ret;
}

RGTexUav RGTexImpl::GetUav()
{
    auto &desc = GetDesc();
    if(desc.arraySize > 1)
    {
        return GetUav(0, 0, desc.arraySize);
    }
    return GetUav(0, 0);
}

RGTexUav RGTexImpl::GetUav(uint32_t mipLevel, uint32_t arrayLayer)
{
    RGTexUav ret;
    ret.resource_ = this;
    ret.desc_ = RGTexUav::NonArrayView{ mipLevel, arrayLayer };
    return ret;
}

RGTexUav RGTexImpl::GetUav(uint32_t mipLevel, uint32_t arrayLayer, uint32_t layerCount)
{
    RGTexUav ret;
    ret.resource_ = this;
    ret.desc_ = RGTexUav::ArrayView{ mipLevel, arrayLayer, layerCount };
    return ret;
}

RGTexRtv RGTexImpl::GetRtv(uint32_t mipLevel, uint32_t arrayLayer)
{
    RGTexRtv ret;
    ret.resource_ = this;
    ret.mipLevel_ = mipLevel;
    ret.arrayLayer_ = arrayLayer;
    return ret;
}

RGTexDsv RGTexImpl::GetDsv(uint32_t mipLevel, uint32_t arrayLayer)
{
    RGTexDsv ret;
    ret.resource_ = this;
    ret.mipLevel_ = mipLevel;
    ret.arrayLayer_ = arrayLayer;
    return ret;
}

RTRC_END
