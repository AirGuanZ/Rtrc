#pragma once

#include <tbb/spin_rw_mutex.h>

#include <Rtrc/Graphics/Object/BatchReleaseHelper.h>

RTRC_BEGIN

class SamplerManager;

class Sampler : public Uncopyable
{
public:

    Sampler() = default;
    ~Sampler();

    Sampler(Sampler &&other) noexcept;
    Sampler &operator=(Sampler &&other) noexcept;

    void Swap(Sampler &other) noexcept;

    const RHI::SamplerPtr &GetRHIObject() const;
    operator const RHI::SamplerPtr &() const;

private:

    friend class SamplerManager;

    SamplerManager *manager_ = nullptr;
    RHI::SamplerPtr rhiSampler_;
};

class SamplerManager : public Uncopyable
{
public:

    SamplerManager(HostSynchronizer &hostSync, RHI::DevicePtr device);

    RC<Sampler> CreateSampler(const RHI::SamplerDesc &desc);

    void _rtrcReleaseInternal(Sampler &sampler);

private:

    void ReleaseImpl(RHI::SamplerPtr sampler);

    RHI::DevicePtr device_;
    BatchReleaseHelper<RHI::SamplerPtr> releaser_;

    std::map<RHI::SamplerDesc, RHI::SamplerPtr> cache_;
    tbb::spin_rw_mutex cacheMutex_;

    std::vector<RHI::SamplerPtr> pendingReleaseSampler_;
    tbb::spin_mutex pendingReleaseSamplerMutex_;
};

inline Sampler::~Sampler()
{
    if(manager_)
    {
        manager_->_rtrcReleaseInternal(*this);
    }
}

inline Sampler::Sampler(Sampler &&other) noexcept
    : Sampler()
{
    Swap(other);
}
inline Sampler &Sampler::operator=(Sampler &&other) noexcept
{
    Swap(other);
    return *this;
}

inline const RHI::SamplerPtr &Sampler::GetRHIObject() const
{
    return rhiSampler_;
}

inline Sampler::operator const ReferenceCountedPtr<RHI::Sampler>&() const
{
    return rhiSampler_;
}

RTRC_END
