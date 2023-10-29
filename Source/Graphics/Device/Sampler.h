#pragma once

#include <Graphics/Device/GeneralGPUObject.h>
#include <Core/Container/ObjectCache.h>

RTRC_BEGIN

class SamplerManager;

class Sampler : public GeneralGPUObject<RHI::SamplerPtr>, public InObjectCache
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
    ObjectCache<RHI::SamplerDesc, Sampler, true, false> cache_;
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
        ret->rhiObject_ = device_->CreateSampler(desc);
        ret->manager_ = this;
        return ret;
    });
}

RTRC_END
