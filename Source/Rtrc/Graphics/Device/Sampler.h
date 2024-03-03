#pragma once

#include <Rtrc/Graphics/Device/GeneralGPUObject.h>
#include <Rtrc/Core/Container/Cache/SharedObjectCache.h>

RTRC_BEGIN

class SamplerManager;

class Sampler : public GeneralGPUObject<RHI::SamplerRPtr>, public InSharedObjectCache
{
    friend class SamplerManager;
};

class SamplerManager : public GeneralGPUObjectManager
{
public:

    SamplerManager(RHI::Device *device, DeviceSynchronizer &sync);

    RC<Sampler> CreateSampler(const RHI::SamplerDesc &desc);

private:

    RHI::Device *device_;
    SharedObjectCache<RHI::SamplerDesc, Sampler, true, false> cache_;
};

inline SamplerManager::SamplerManager(RHI::Device *device, DeviceSynchronizer &sync)
    : GeneralGPUObjectManager(sync), device_(std::move(device))
{
    
}

inline RC<Sampler> SamplerManager::CreateSampler(const RHI::SamplerDesc &desc)
{
    return cache_.GetOrCreate(desc, [&]
    {
        auto ret = MakeRC<Sampler>();
        ret->rhiObject_ = device_->CreateSampler(desc).ToRC();
        ret->manager_ = this;
        return ret;
    });
}

RTRC_END
