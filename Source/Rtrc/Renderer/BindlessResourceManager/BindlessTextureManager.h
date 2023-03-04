#pragma once

#include <shared_mutex>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Utility/Container/RangeSet.h>

RTRC_BEGIN

class BindlessTextureManager;

// Currently, only texture srv is supported
class BindlessTextureEntry
{
public:

    void Set(RC<Texture> texture);
    void Set(uint32_t index, RC<Texture> texture);
    void Clear(uint32_t index = 0);

    uint32_t GetOffset() const;
    uint32_t GetCount() const;
    operator bool() const;

private:

    friend class BindlessTextureManager;

    BindlessTextureManager *manager_ = nullptr;
    uint32_t offset_ = 0;
    uint32_t count_ = 0;
};

class BindlessTextureManager : public Uncopyable
{
public:

    BindlessTextureManager(Device &device, uint32_t initialArraySize = 64);

    BindlessTextureEntry Allocate(uint32_t count = 1);
    void Free(BindlessTextureEntry handle);

    const RC<BindingGroupLayout> &GetBindingGroupLayout() const;
    RC<BindingGroup> GetBindingGroup() const;

    void _internalSet(uint32_t index, RC<Texture> texture);
    void _internalClear(uint32_t index);

private:

    void Expand();

    Device                &device_;
    RC<BindingGroupLayout> bindingGroupLayout_;

    // TODO: thread-local allocator
    mutable std::shared_mutex mutex_;
    RangeSet                  freeRanges_;
    std::vector<RC<Texture>>  boundTextures_;
    RC<BindingGroup>          bindingGroup_;
    uint32_t                  currentArraySize_;
};

inline const RC<BindingGroupLayout> &BindlessTextureManager::GetBindingGroupLayout() const
{
    return bindingGroupLayout_;
}

inline RC<BindingGroup> BindlessTextureManager::GetBindingGroup() const
{
    std::shared_lock lock(mutex_);
    return bindingGroup_;
}

RTRC_END
