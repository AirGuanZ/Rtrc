#include <shared_mutex>

#include <Rtrc/Graphics/Object/Sampler.h>

RTRC_BEGIN

SamplerManager::SamplerManager(HostSynchronizer &hostSync, RHI::DevicePtr device)
    : device_(std::move(device)), releaser_(hostSync)
{
    using ReleaseDataList = BatchReleaseHelper<RHI::SamplerPtr>::DataList;
    releaser_.SetReleaseCallback([this](int, ReleaseDataList &dataList)
    {
        std::unique_lock lock(cacheMutex_);
        for(auto &data : dataList)
        {
            if(auto it = cache_.find(data->GetDesc()); it != cache_.end())
            {
                if(it->second == data)
                {
                    cache_.erase(it);
                }
            }
        }
    });

    releaser_.SetPreNewBatchCallback([this]
    {
        std::vector<RHI::SamplerPtr> pendingReleaseSamplers;
        {
            std::lock_guard lock(pendingReleaseSamplerMutex_);
            pendingReleaseSamplers.swap(pendingReleaseSampler_);
        }
        for(auto &s : pendingReleaseSamplers)
        {
            ReleaseImpl(s);
        }
    });
}

RC<Sampler> SamplerManager::CreateSampler(const RHI::SamplerDesc &desc)
{
    auto ret = MakeRC<Sampler>();

    {
        std::shared_lock lock(cacheMutex_);
        if(auto it = cache_.find(desc); it != cache_.end())
        {
            ret->rhiSampler_ = it->second;
            return ret;
        }
    }

    RHI::SamplerPtr rhiSampler;
    
    {
        std::unique_lock lock(cacheMutex_);
        if(auto it = cache_.find(desc); it != cache_.end())
        {
            rhiSampler = it->second;
        }
        else
        {
            rhiSampler = device_->CreateSampler(desc);
            cache_.insert({ desc, rhiSampler });
        }
    }
    ret->rhiSampler_ = std::move(rhiSampler);
    return ret;
}

void SamplerManager::_rtrcReleaseInternal(Sampler &sampler)
{
    if(releaser_.GetHostSynchronizer().IsInOwnerThread())
    {
        ReleaseImpl(std::move(sampler.rhiSampler_));
    }
    else
    {
        std::lock_guard lock(pendingReleaseSamplerMutex_);
        pendingReleaseSampler_.push_back(std::move(sampler.rhiSampler_));
    }
    sampler.rhiSampler_ = {};
}

void SamplerManager::ReleaseImpl(RHI::SamplerPtr sampler)
{
    releaser_.AddToCurrentBatch(std::move(sampler));
}

RTRC_END
