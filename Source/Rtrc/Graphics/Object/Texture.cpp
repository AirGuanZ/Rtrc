#include <mutex>

#include <Rtrc/Graphics/Object/Texture.h>

RTRC_BEGIN

Texture2D::Texture2D(TextureManager *manager, RHI::TexturePtr rhiTexture)
    : manager_(manager), rhiTexture_(std::move(rhiTexture))
{
    auto &desc = rhiTexture_->GetDesc();
    width_ = desc.width;
    height_ = desc.height;
    arrayLayerCount_ = desc.arraySize;
    mipLevelCount_ = desc.mipLevels;
}

TextureManager::TextureManager(HostSynchronizer &hostSync, RHI::DevicePtr device)
    : batchReleaser_(hostSync), device_(std::move(device))
{
    batchReleaser_.SetPreNewBatchCallback([this]
    {
        std::list<ReleaseRecord> pendingReleaseTextures;
        {
            std::lock_guard lock(pendingReleaseTexturesMutex_);
            pendingReleaseTextures.swap(pendingReleaseTextures_);
        }
        for(auto &b : pendingReleaseTextures)
        {
            ReleaseImpl(b);
        }
    });

    batchReleaser_.SetReleaseCallback([this](int batchIndex, BatchReleaser::DataList &dataList)
    {
        for(auto it = reuseRecords_.begin(); it != reuseRecords_.end();)
        {
            if(it->second.releaseBatchIndex == batchIndex)
            {
                it = reuseRecords_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        std::lock_guard lock(garbageMutex_);
        for(auto &data : dataList)
        {
            if(data.rhiTexture)
            {
                garbageTextures_.push_back(std::move(data.rhiTexture));
            }
        }
    });
}

RC<Texture2D> TextureManager::CreateTexture2D(
    uint32_t                       width,
    uint32_t                       height,
    uint32_t                       arraySize,
    uint32_t                       mipLevelCount,
    RHI::Format                    format,
    RHI::TextureUsageFlag          usages,
    RHI::QueueConcurrentAccessMode concurrentMode,
    uint32_t                       sampleCount,
    bool                           allowReuse)
{
    if(mipLevelCount <= 0)
    {
        mipLevelCount = 1;
        uint32_t t = std::max(width, height);
        while(t > 1)
        {
            ++mipLevelCount;
            t <<= 1;
        }
    }

    const RHI::TextureDesc desc =
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = format,
        .width                = width,
        .height               = height,
        .arraySize            = arraySize,
        .mipLevels            = mipLevelCount,
        .sampleCount          = sampleCount,
        .usage                = usages,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = concurrentMode
    };

    // Reuse

    allowReuse &= batchReleaser_.GetHostSynchronizer().IsInOwnerThread();
    if(allowReuse)
    {
        ReuseRecord reuseRecord = { -1, {} };
        {
            std::lock_guard lock(reuseRecordsMutex_);
            if(auto it = reuseRecords_.find(desc); it != reuseRecords_.end())
            {
                reuseRecord = it->second;
                reuseRecords_.erase(it);
            }
        }
        ReleaseRecord &releaseRecord = *reuseRecord.releaseRecordIt;

        Texture2D tex(this, std::move(releaseRecord.rhiTexture));
        tex.unsyncAccesses_ = std::move(releaseRecord.unsyncAccesses);
        assert(!tex.unsyncAccesses_.IsEmpty());
        releaseRecord.rhiTexture = {};
        releaseRecord.unsyncAccesses = {};
        return MakeRC<Texture2D>(std::move(tex));
    }

    // Create

    auto rhiTexture = device_->CreateTexture(desc);
    Texture2D tex(this, std::move(rhiTexture));
    return MakeRC<Texture2D>(std::move(tex));
}

void TextureManager::GC()
{
    std::vector<RHI::TexturePtr> garbageTextures;
    {
        std::lock_guard lock(garbageMutex_);
        garbageTextures.swap(garbageTextures_);
    }
}

void TextureManager::_rtrcReleaseInternal(Texture2D &tex)
{
    if(batchReleaser_.GetHostSynchronizer().IsInOwnerThread())
    {
        ReleaseImpl({ std::move(tex.rhiTexture_), std::move(tex.unsyncAccesses_), tex.allowReuse_ });
    }
    else
    {
        std::lock_guard lock(pendingReleaseTexturesMutex_);
        pendingReleaseTextures_.push_back({ std::move(tex.rhiTexture_), std::move(tex.unsyncAccesses_), tex.allowReuse_ });
    }
}

void TextureManager::ReleaseImpl(ReleaseRecord tex)
{
    const bool allowReuse = tex.allowReuse;
    auto recordIt = batchReleaser_.AddToCurrentBatch(std::move(tex));
    const ReleaseRecord &releaseRecord = *recordIt;
    const int batchIndex = batchReleaser_.GetCurrentBatchIndex();
    if(allowReuse)
    {
        std::lock_guard lock(reuseRecordsMutex_);
        reuseRecords_.insert({ releaseRecord.rhiTexture->GetDesc(), { batchIndex, recordIt } });
    }
}

RTRC_END
