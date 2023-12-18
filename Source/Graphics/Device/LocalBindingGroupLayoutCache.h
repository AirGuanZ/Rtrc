#pragma once

#include <atomic>

#include <Graphics/Device/BindingGroupDSL.h>
#include <Core/SmartPointer/ObserverPtr.h>

RTRC_BEGIN

class Device;

class CachedBindingGroupLayoutStorage : public Uncopyable
{
public:

    CachedBindingGroupLayoutStorage()
    {
        static std::atomic<uint32_t> nextIndex = 0;
        index_ = nextIndex++;
    }
    
private:

    friend class BindingGroupLayoutCache;

    uint32_t index_;
};

template<typename T>
class CachedBindingGroupLayoutHandle
{
public:

    CachedBindingGroupLayoutHandle(Ref<Device> device, CachedBindingGroupLayoutStorage *storage)
        : device_(device), storage_(storage)
    {
        
    }

    CachedBindingGroupLayoutHandle(const CachedBindingGroupLayoutHandle &) = default;
    CachedBindingGroupLayoutHandle &operator=(const CachedBindingGroupLayoutHandle &) = default;

    operator RC<BindingGroupLayout>() const
    {
        return Get();
    }

    BindingGroupLayout *operator->() const
    {
        return Get().get();
    }

    RC<BindingGroupLayout> Get() const;

private:

    Ref<Device> device_;
    CachedBindingGroupLayoutStorage *storage_;
};

class BindingGroupLayoutCache : public Uncopyable
{
public:

    explicit BindingGroupLayoutCache(Ref<Device> device)
        : device_(device)
    {
        
    }

    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroupLayout> Get(CachedBindingGroupLayoutStorage *storage);

private:

    Ref<Device> device_;
    tbb::spin_rw_mutex mutex_;
    std::vector<RC<BindingGroupLayout>> layouts_;
};

#define RTRC_STATIC_BINDING_GROUP_LAYOUT(DEVICE, TYPE, NAME) RTRC_STATIC_BINDING_GROUP_LAYOUT_IMPL(DEVICE, TYPE, NAME)
#define RTRC_STATIC_BINDING_GROUP_LAYOUT_IMPL(DEVICE, TYPE, NAME)                                           \
    static ::Rtrc::CachedBindingGroupLayoutStorage _staticBindingGroupLayoutStorage##NAME;                  \
    ::Rtrc::CachedBindingGroupLayoutHandle<TYPE> NAME{ (DEVICE), &_staticBindingGroupLayoutStorage##NAME }

RTRC_END
