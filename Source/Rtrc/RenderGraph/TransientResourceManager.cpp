#include <Rtrc/RenderGraph/TransientResourceManager.h>

RTRC_RG_BEGIN

ResourceLifetimeManager::ResourceLifetimeManager(int resourceCount, int passCount)
    : resourceCount_(resourceCount), passCount_(passCount)
{
    maskCountPerResource_ = (passCount + MASK_BITS - 1) / MASK_BITS;
    lifeTimeMasks_.resize(maskCountPerResource_ * resourceCount);
}

int ResourceLifetimeManager::GetResourceCount() const
{
    return resourceCount_;
}

int ResourceLifetimeManager::GetMaskCountPerResource() const
{
    return maskCountPerResource_;
}

int ResourceLifetimeManager::GetPassCount() const
{
    return passCount_;
}

void ResourceLifetimeManager::SetBit(int resourceIndex, int passIndex)
{
    const int maskIndex = resourceIndex * maskCountPerResource_ + passIndex / MASK_BITS;
    const int inMaskIndex = passIndex % MASK_BITS;
    lifeTimeMasks_[maskIndex] |= 1 << inMaskIndex;
}

bool ResourceLifetimeManager::GetBit(int resourceIndex, int passIndex) const
{
    const int maskIndex = resourceIndex * maskCountPerResource_ + passIndex / MASK_BITS;
    const int inMaskIndex = passIndex % MASK_BITS;
    return (lifeTimeMasks_[maskIndex] & (1 << inMaskIndex)) != 0;
}

Span<ResourceLifetimeManager::Mask> ResourceLifetimeManager::GetBits(int resourceIndex) const
{
    return Span(&lifeTimeMasks_[resourceIndex * maskCountPerResource_], maskCountPerResource_);
}

TransientResourceManager::TransientResourceManager(RHI::DevicePtr device, Strategy strategy, int frameCount)
{
    assert(strategy == Strategy::ReuseAcrossFrame);
    reuseAcrossFrame_ = MakeBox<TransientResourceManagerWithoutReuse>(std::move(device), frameCount);
}

void TransientResourceManager::BeginFrame()
{
    assert(reuseAcrossFrame_);
    reuseAcrossFrame_->BeginFrame();
}

void TransientResourceManager::Allocate(
    const ResourceLifetimeManager &lifetimeManager,
    Span<ResourceDesc>             lifeTimeIndexToDesc,
    Span<int>                      lifeTimeIndexToResourceIndex,
    ExecutableResources           &resources)
{
    assert(reuseAcrossFrame_);
    RTRC_MAYBE_UNUSED(lifetimeManager);
    for(size_t lifeTimeIndex = 0; lifeTimeIndex < lifeTimeIndexToDesc.size(); ++lifeTimeIndex)
    {
        const int resourceIndex = lifeTimeIndexToResourceIndex[lifeTimeIndex];
        lifeTimeIndexToDesc[lifeTimeIndex].Match(
            [&](const RHI::BufferDesc &bufferDesc)
            {
                resources.indexToBuffer[resourceIndex].buffer = reuseAcrossFrame_->AllocateBuffer(bufferDesc);
            },
            [&](const RHI::Texture2DDesc &tex2DDesc)
            {
                resources.indexToTexture[resourceIndex].texture = reuseAcrossFrame_->AllocateTexture2D(tex2DDesc);
            });
    }
}

RTRC_RG_END
