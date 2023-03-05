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

    BindlessTextureEntry() = default;

    BindlessTextureEntry(const BindlessTextureEntry &) = default;
    BindlessTextureEntry &operator=(const BindlessTextureEntry &) = default;

    BindlessTextureEntry(BindlessTextureEntry &&other) noexcept;
    BindlessTextureEntry &operator=(BindlessTextureEntry &&other) noexcept;

    void Swap(BindlessTextureEntry &other) noexcept;

    void Set(RC<Texture> texture);
    void Set(uint32_t index, RC<Texture> texture);
    void Clear(uint32_t index = 0);

    uint32_t GetOffset() const;
    uint32_t GetCount() const;
    operator bool() const;

private:

    friend class BindlessTextureManager;

    struct Impl : ReferenceCounted, Uncopyable
    {
        ~Impl();

        BindlessTextureManager *manager = nullptr;
        uint32_t offset = 0;
        uint32_t count = 0;
    };

    ReferenceCountedPtr<Impl> impl_;
};

class BindlessTextureManager : public Uncopyable
{
public:

    BindlessTextureManager(Device &device, uint32_t initialArraySize = 64);

    BindlessTextureEntry Allocate(uint32_t count = 1);

    const RC<BindingGroupLayout> &GetBindingGroupLayout() const;
    RC<BindingGroup> GetBindingGroup() const;

    void _internalSet(uint32_t index, RC<Texture> texture);
    void _internalClear(uint32_t index);

private:

    friend struct BindlessTextureEntry::Impl;

    void _internalFree(BindlessTextureEntry::Impl *entry);

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

inline BindlessTextureEntry::BindlessTextureEntry(BindlessTextureEntry &&other) noexcept
    : BindlessTextureEntry()
{
    Swap(other);
}

inline BindlessTextureEntry &BindlessTextureEntry::operator=(BindlessTextureEntry &&other) noexcept
{
    Swap(other);
    return *this;
}

inline void BindlessTextureEntry::Swap(BindlessTextureEntry &other) noexcept
{
    impl_.Swap(other.impl_);
}

inline void BindlessTextureEntry::Set(RC<Texture> texture)
{
    Set(0, std::move(texture));
}

inline void BindlessTextureEntry::Set(uint32_t index, RC<Texture> texture)
{
    assert(impl_ && index < impl_->count);
    impl_->manager->_internalSet(impl_->offset + index, std::move(texture));
}

inline void BindlessTextureEntry::Clear(uint32_t index)
{
    assert(impl_ && index < impl_->count);
    impl_->manager->_internalClear(impl_->offset + index);
}

inline uint32_t BindlessTextureEntry::GetOffset() const
{
    return impl_->offset;
}

inline uint32_t BindlessTextureEntry::GetCount() const
{
    return impl_->count;
}

inline BindlessTextureEntry::operator bool() const
{
    return impl_ != nullptr;
}

inline BindlessTextureEntry::Impl::~Impl()
{
    assert(manager);
    manager->_internalFree(this);
}

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
