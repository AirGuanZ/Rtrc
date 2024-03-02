#pragma once

#include <shared_mutex>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Core/Container/RangeSet.h>
#include <Rtrc/Core/SmartPointer/ObserverPtr.h>

RTRC_BEGIN

enum class BindlessResourceType
{
    Texture,          // Sampled texture
    StructuredBuffer, // Structured buffer
};

class Texture;
class Device;

template<BindlessResourceType ResourceType>
class BindlessResourceManager;

template<BindlessResourceType ResourceType>
class BindlessResourceEntry
{
public:

    using Manager = BindlessResourceManager<ResourceType>;
    using Resource = std::conditional_t<ResourceType == BindlessResourceType::Texture, Texture, SubBuffer>;
    using ResourceSrv = std::conditional_t<ResourceType == BindlessResourceType::Texture, TextureSrv, BufferSrv>;

    BindlessResourceEntry() = default;

    BindlessResourceEntry(const BindlessResourceEntry &) = default;
    BindlessResourceEntry &operator=(const BindlessResourceEntry &) = default;

    BindlessResourceEntry(BindlessResourceEntry &&other) noexcept;
    BindlessResourceEntry &operator=(BindlessResourceEntry &&other) noexcept;

    void Swap(BindlessResourceEntry &other) noexcept;

    void Set(RC<Resource> resource);
    void Set(uint32_t index, RC<Resource> resource);

    void Set(const ResourceSrv &srv);
    void Set(uint32_t index, const ResourceSrv &srv);

    void Clear(uint32_t index = 0);

    uint32_t GetOffset() const;
    uint32_t GetCount() const;
    operator bool() const;

private:

    friend class BindlessResourceManager<ResourceType>;

    struct Impl : ReferenceCounted, Uncopyable
    {
        ~Impl();

        Manager *manager = nullptr;
        uint32_t offset = 0;
        uint32_t count = 0;
    };

    ReferenceCountedPtr<Impl> impl_;
};

template<BindlessResourceType ResourceType>
class BindlessResourceManager : public Uncopyable
{
public:

    using Entry = BindlessResourceEntry<ResourceType>;
    using Resource = typename Entry::Resource;
    using ResourceSrv = typename Entry::ResourceSrv;

    explicit BindlessResourceManager(Ref<Device> device, uint32_t initialArraySize = 64, uint32_t maxArraySize = 4096);

    Entry Allocate(uint32_t count = 1);

    const RC<BindingGroupLayout> &GetBindingGroupLayout() const;
    RC<BindingGroup> GetBindingGroup() const;

    void _internalSet(uint32_t index, RC<Resource> resource);
    void _internalSet(uint32_t index, const ResourceSrv &srv);
    void _internalClear(uint32_t index);

private:

    friend struct Entry::Impl;

    struct SharedData
    {
        std::shared_mutex mutex;
        RangeSet freeRanges;
    };

    void _internalFree(typename Entry::Impl *entry);

    void Expand();

    Ref<Device>            device_;
    RC<BindingGroupLayout> bindingGroupLayout_;

    // TODO: thread-local allocator
    RC<SharedData>            sharedData_;
    std::vector<RC<Resource>> boundResources_;
    RC<BindingGroup>          bindingGroup_;
    uint32_t                  currentArraySize_;
    uint32_t                  maxArraySize_;
};

template<BindlessResourceType ResourceType>
BindlessResourceEntry<ResourceType>::BindlessResourceEntry(BindlessResourceEntry &&other) noexcept
    : BindlessResourceEntry()
{
    Swap(other);
}

template<BindlessResourceType ResourceType>
BindlessResourceEntry<ResourceType> &
    BindlessResourceEntry<ResourceType>::operator=(BindlessResourceEntry &&other) noexcept
{
    Swap(other);
    return *this;
}

template<BindlessResourceType ResourceType>
void BindlessResourceEntry<ResourceType>::Swap(BindlessResourceEntry &other) noexcept
{
    impl_.Swap(other.impl_);
}

template<BindlessResourceType ResourceType>
void BindlessResourceEntry<ResourceType>::Set(RC<Resource> resource)
{
    this->Set(0, std::move(resource));
}

template<BindlessResourceType ResourceType>
void BindlessResourceEntry<ResourceType>::Set(uint32_t index, RC<Resource> resource)
{
    assert(impl_ && index < impl_->count);
    impl_->manager->_internalSet(impl_->offset + index, std::move(resource));
}

template<BindlessResourceType ResourceType>
void BindlessResourceEntry<ResourceType>::Set(const ResourceSrv &srv)
{
    this->Set(0, srv);
}

template<BindlessResourceType ResourceType>
void BindlessResourceEntry<ResourceType>::Set(uint32_t index, const ResourceSrv &srv)
{
    assert(impl_ && index < impl_->count);
    impl_->manager->_internalSet(impl_->offset + index, srv);
}

template<BindlessResourceType ResourceType>
void BindlessResourceEntry<ResourceType>::Clear(uint32_t index)
{
    assert(impl_ && index < impl_->count);
    impl_->manager->_internalClear(impl_->offset + index);
}

template<BindlessResourceType ResourceType>
uint32_t BindlessResourceEntry<ResourceType>::GetOffset() const
{
    return impl_->offset;
}

template<BindlessResourceType ResourceType>
uint32_t BindlessResourceEntry<ResourceType>::GetCount() const
{
    return impl_->count;
}

template<BindlessResourceType ResourceType>
BindlessResourceEntry<ResourceType>::operator bool() const
{
    return impl_ != nullptr;
}

template<BindlessResourceType ResourceType>
BindlessResourceEntry<ResourceType>::Impl::~Impl()
{
    assert(manager);
    manager->_internalFree(this);
}

template<BindlessResourceType ResourceType>
const RC<BindingGroupLayout> &BindlessResourceManager<ResourceType>::GetBindingGroupLayout() const
{
    return bindingGroupLayout_;
}

template<BindlessResourceType ResourceType>
RC<BindingGroup> BindlessResourceManager<ResourceType>::GetBindingGroup() const
{
    std::shared_lock lock(sharedData_->mutex);
    return bindingGroup_;
}

template<BindlessResourceType ResourceType>
BindlessResourceManager<ResourceType>::BindlessResourceManager(
    Ref<Device> device, uint32_t initialArraySize, uint32_t maxArraySize)
    : device_(device)
{
    sharedData_ = MakeRC<SharedData>();

    BindingGroupLayout::Desc layoutDesc;
    layoutDesc.variableArraySize = true;
    BindingGroupLayout::BindingDesc &bindingDesc = layoutDesc.bindings.emplace_back();
    if constexpr(ResourceType == BindlessResourceType::Texture)
    {
        bindingDesc.type = RHI::BindingType::Texture;
    }
    else
    {
        bindingDesc.type = RHI::BindingType::StructuredBuffer;
    }
    bindingDesc.stages    = RHI::ShaderStage::All;
    bindingDesc.arraySize = maxArraySize;
    bindingDesc.bindless  = true;
    bindingGroupLayout_ = device_->CreateBindingGroupLayout(layoutDesc);

    sharedData_->freeRanges = RangeSet(initialArraySize);
    boundResources_.resize(initialArraySize);
    bindingGroup_ = bindingGroupLayout_->CreateBindingGroup(static_cast<int>(initialArraySize));
    currentArraySize_ = initialArraySize;
    maxArraySize_ = maxArraySize;
}

template<BindlessResourceType ResourceType>
BindlessResourceEntry<ResourceType> BindlessResourceManager<ResourceType>::Allocate(uint32_t count)
{
    std::lock_guard lock(sharedData_->mutex);
    while(true)
    {
        const RangeSet::Index offset = sharedData_->freeRanges.Allocate(count);
        if(offset == RangeSet::NIL)
        {
            Expand();
            continue;
        }
        Entry entry;
        entry.impl_ = MakeReferenceCountedPtr<typename Entry::Impl>();
        entry.impl_->manager = this;
        entry.impl_->offset = offset;
        entry.impl_->count = count;
        return entry;
    }
}

template<BindlessResourceType ResourceType>
void BindlessResourceManager<ResourceType>::_internalSet(uint32_t index, RC<Resource> resource)
{
    if constexpr(ResourceType == BindlessResourceType::Texture)
    {
        bindingGroup_->Set(0, static_cast<int>(index), resource);
    }
    else
    {
        static_assert(ResourceType == BindlessResourceType::StructuredBuffer);
        const RC<SubBuffer> &rsc = resource;
        bindingGroup_->Set(0, static_cast<int>(index), rsc->GetStructuredSrv());
    }
    boundResources_[index] = std::move(resource);
}

template<BindlessResourceType ResourceType>
void BindlessResourceManager<ResourceType>::_internalSet(uint32_t index, const ResourceSrv &srv)
{
    if constexpr(ResourceType == BindlessResourceType::Texture)
    {
        const TextureSrv &textureSrv = srv;
        bindingGroup_->Set(0, static_cast<int>(index), textureSrv);
        boundResources_[index] = textureSrv.GetTexture();
    }
    else
    {
        static_assert(ResourceType == BindlessResourceType::StructuredBuffer);
        const BufferSrv &bufferSrv = srv;
        bindingGroup_->Set(0, static_cast<int>(index), bufferSrv);
        boundResources_[index] = bufferSrv.GetBuffer();
    }
}

template<BindlessResourceType ResourceType>
void BindlessResourceManager<ResourceType>::_internalClear(uint32_t index)
{
    boundResources_[index].reset();
}

template<BindlessResourceType ResourceType>
void BindlessResourceManager<ResourceType>::_internalFree(typename Entry::Impl *entry)
{
    assert(entry && entry->manager == this);
    device_->GetSynchronizer().OnFrameComplete([offset = entry->offset, count = entry->count, s = sharedData_]
    {
        std::lock_guard lock(s->mutex);
        s->freeRanges.Free(offset, offset + count);
    });
    std::lock_guard lock(sharedData_->mutex);
    for(uint32_t i = entry->offset; i < entry->offset + entry->count; ++i)
    {
        boundResources_[i].reset();
    }
}

template<BindlessResourceType ResourceType>
void BindlessResourceManager<ResourceType>::Expand()
{
    const uint32_t newArraySize = (std::min)(currentArraySize_ << 1, maxArraySize_);
    if(newArraySize == currentArraySize_)
    {
        throw Exception("BindlessResourceManager::Expand: desc array is full");
    }
    RC<BindingGroup> newBindingGroup = bindingGroupLayout_->CreateBindingGroup(static_cast<int>(newArraySize));
    device_->CopyBindings(newBindingGroup, 0, 0, bindingGroup_, 0, 0, currentArraySize_);
    sharedData_->freeRanges.Free(currentArraySize_, newArraySize);
    boundResources_.resize(newArraySize);
    bindingGroup_ = std::move(newBindingGroup);
    currentArraySize_ = newArraySize;
}

using BindlessTextureEntry = BindlessResourceEntry<BindlessResourceType::Texture>;
using BindlessBufferEntry = BindlessResourceEntry<BindlessResourceType::StructuredBuffer>;

using BindlessTextureManager = BindlessResourceManager<BindlessResourceType::Texture>;
using BindlessBufferManager = BindlessResourceManager<BindlessResourceType::StructuredBuffer>;

RTRC_END
