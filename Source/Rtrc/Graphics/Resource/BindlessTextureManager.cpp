#include <Rtrc/Graphics/Resource/BindlessTextureManager.h>

RTRC_BEGIN

BindlessTextureManager::BindlessTextureManager(Device &device, uint32_t initialArraySize)
    : device_(device)
{
    BindingGroupLayout::Desc layoutDesc;
    layoutDesc.variableArraySize = true;
    BindingGroupLayout::BindingDesc &bindingDesc = layoutDesc.bindings.emplace_back();
    bindingDesc.type     = RHI::BindingType::Texture;
    bindingDesc.stages   = RHI::ShaderStage::All;
    bindingDesc.bindless = true;
    bindingGroupLayout_ = device.CreateBindingGroupLayout(layoutDesc);

    freeRanges_ = RangeSet(initialArraySize);
    boundTextures_.resize(initialArraySize);
    bindingGroup_ = bindingGroupLayout_->CreateBindingGroup(static_cast<int>(initialArraySize));
    currentArraySize_ = initialArraySize;
}

BindlessTextureEntry BindlessTextureManager::Allocate(uint32_t count)
{
    std::lock_guard lock(mutex_);
    while(true)
    {
        const RangeSet::Index offset = freeRanges_.Allocate(count);
        if(offset == RangeSet::NIL)
        {
            Expand();
            continue;
        }
        BindlessTextureEntry entry;
        entry.impl_ = MakeReferenceCountedPtr<BindlessTextureEntry::Impl>();
        entry.impl_->manager = this;
        entry.impl_->offset = offset;
        entry.impl_->count = count;
        return entry;
    }
}

void BindlessTextureManager::_internalSet(uint32_t index, RC<Texture> texture)
{
    bindingGroup_->Set(0, static_cast<int>(index), texture);
    boundTextures_[index] = std::move(texture);
}

void BindlessTextureManager::_internalClear(uint32_t index)
{
    boundTextures_[index].reset();
}

void BindlessTextureManager::_internalFree(BindlessTextureEntry::Impl *entry)
{
    assert(entry && entry->manager == this);
    device_.GetSynchronizer().OnFrameComplete([offset = entry->offset, count = entry->count, this]
    {
        std::lock_guard lock(mutex_);
        freeRanges_.Free(offset, offset + count);
    });
    std::lock_guard lock(mutex_);
    for(uint32_t i = entry->offset; i < entry->offset + entry->count; ++i)
    {
        boundTextures_[i].reset();
    }
}

void BindlessTextureManager::Expand()
{
    const uint32_t newArraySize = currentArraySize_ << 1;
    RC<BindingGroup> newBindingGroup = bindingGroupLayout_->CreateBindingGroup(static_cast<int>(newArraySize));
    device_.CopyBindings(newBindingGroup, 0, 0, bindingGroup_, 0, 0, currentArraySize_);
    freeRanges_.Free(currentArraySize_, newArraySize);
    boundTextures_.resize(newArraySize);
    bindingGroup_ = std::move(newBindingGroup);
    currentArraySize_ = newArraySize;
}

RTRC_END
