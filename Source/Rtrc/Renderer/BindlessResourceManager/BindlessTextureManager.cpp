#include <Rtrc/Renderer/BindlessResourceManager/BindlessTextureManager.h>

RTRC_BEGIN

void BindlessTextureEntry::Set(RC<Texture> texture)
{
    Set(0, std::move(texture));
}

void BindlessTextureEntry::Set(uint32_t index, RC<Texture> texture)
{
    assert(index < count_);
    manager_->_internalSet(offset_ + index, std::move(texture));
}

void BindlessTextureEntry::Clear(uint32_t index)
{
    assert(index < count_);
    manager_->_internalClear(offset_ + index);
}

uint32_t BindlessTextureEntry::GetOffset() const
{
    return offset_;
}

uint32_t BindlessTextureEntry::GetCount() const
{
    return count_;
}

BindlessTextureEntry::operator bool() const
{
    return count_ > 0;
}

BindlessTextureManager::BindlessTextureManager(Device &device, uint32_t initialArraySize)
    : device_(device)
{
    BindingGroupLayout::Desc layoutDesc;
    layoutDesc.variableArraySize = true;
    BindingGroupLayout::BindingDesc &bindingDesc = layoutDesc.bindings.emplace_back();
    bindingDesc.type     = RHI::BindingType::Texture2D;
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
        entry.manager_ = this;
        entry.offset_ = offset;
        entry.count_ = count;
        return entry;
    }
}

void BindlessTextureManager::Free(BindlessTextureEntry handle)
{
    assert(handle.manager_ == this);
    std::lock_guard lock(mutex_);
    freeRanges_.Free(handle.offset_, handle.offset_ + handle.count_);
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
