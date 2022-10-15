#pragma once

#include <deque>

#include <Rtrc/Graphics/Object/BatchReleaseHelper.h>

RTRC_BEGIN

class GeneralGPUObjectManager;

template<typename RHIObjectPtr>
class GeneralGPUObject : public Uncopyable
{
public:

    ~GeneralGPUObject();

    const RHIObjectPtr &GetRHIObject() const;

protected:

    friend class GeneralGPUObjectManager;

    GeneralGPUObjectManager *manager_ = nullptr;
    RHIObjectPtr rhiObject_;
};

/*
    Simple delayed rhi object releasing
*/
class GeneralGPUObjectManager
{
public:

    GeneralGPUObjectManager(HostSynchronizer &hostSync, RHI::DevicePtr device);

    const RHI::DevicePtr &GetDevice() const;

    template<typename RHIObjectPtr>
    void _rtrcInternalRelease(GeneralGPUObject<RHIObjectPtr> &object);

private:

    struct ReleaseRecord
    {
        RHI::RHIObjectPtr object;
    };

    BatchReleaseHelper<ReleaseRecord> releaser_;
    RHI::DevicePtr device_;

    std::deque<ReleaseRecord> pendingReleaseRecords_;
    tbb::spin_mutex pendingReleaseRecordsMutex_;
};

template<typename RHIObjectPtr>
GeneralGPUObject<RHIObjectPtr>::~GeneralGPUObject()
{
    if(manager_)
    {
        manager_->_rtrcInternalRelease(*this);
    }
}

template<typename RHIObjectPtr>
const RHIObjectPtr &GeneralGPUObject<RHIObjectPtr>::GetRHIObject() const
{
    return rhiObject_;
}

inline GeneralGPUObjectManager::GeneralGPUObjectManager(HostSynchronizer &hostSync, RHI::DevicePtr device)
    : releaser_(hostSync), device_(std::move(device))
{
    releaser_.SetPreNewBatchCallback([this]
    {
        std::deque<ReleaseRecord> pendingReleaseRecords;
        {
            std::lock_guard lock(pendingReleaseRecordsMutex_);
            pendingReleaseRecords.swap(pendingReleaseRecords_);
        }
        for(auto &r : pendingReleaseRecords)
            releaser_.AddToCurrentBatch(std::move(r));
    });
}

inline const RHI::DevicePtr &GeneralGPUObjectManager::GetDevice() const
{
    return device_;
}

template<typename RHIObjectPtr>
void GeneralGPUObjectManager::_rtrcInternalRelease(GeneralGPUObject<RHIObjectPtr> &object)
{
    std::lock_guard lock(pendingReleaseRecordsMutex_);
    pendingReleaseRecords_.push_back({ std::move(object.rhiObject_) });
}

RTRC_END
