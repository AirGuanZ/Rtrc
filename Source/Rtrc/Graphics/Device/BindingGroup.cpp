#include <Rtrc/Graphics/Device/BindingGroup.h>

RTRC_BEGIN

BindingGroupManager::BindingGroupManager(
    RHI::DevicePtr device, DeviceSynchronizer &sync, ConstantBufferManagerInterface *defaultConstantBufferManager)
    : device_(std::move(device)), sync_(sync), defaultConstantBufferManager_(defaultConstantBufferManager)
{

}

RC<BindingGroupLayout> BindingGroupManager::CreateBindingGroupLayout(const RHI::BindingGroupLayoutDesc &desc)
{
    return groupLayoutCache_.GetOrCreate(desc, [&]
        {
            auto ret = MakeRC<BindingGroupLayout>();
            ret->manager_ = this;
            ret->rhiLayout_ = device_->CreateBindingGroupLayout(desc);
            for(size_t slot = 0; slot < desc.bindings.size(); ++slot)
            {
                auto &b = desc.bindings[slot];
                ret->nameToSlot_[b.name] = static_cast<int>(slot);
            }
            return ret;
        });
}

RC<BindingGroup> BindingGroupManager::CreateBindingGroup(RC<const BindingGroupLayout> groupLayout)
{
    auto ret = MakeRC<BindingGroup>();
    ret->rhiGroup_ = device_->CreateBindingGroup(groupLayout->rhiLayout_);
    ret->manager_ = this;
    ret->boundObjects_.resize(groupLayout->rhiLayout_->GetDesc().bindings.size());
    ret->layout_ = std::move(groupLayout);
    return ret;
}

RC<BindingLayout> BindingGroupManager::CreateBindingLayout(const BindingLayout::Desc &desc)
{
    return layoutCache_.GetOrCreate(desc, [&]
        {
            RHI::BindingLayoutDesc rhiDesc;
            rhiDesc.groups.reserve(desc.groupLayouts.size());
            for(auto &group : desc.groupLayouts)
            {
                rhiDesc.groups.push_back(group->GetRHIObject());
            }

            auto ret = MakeRC<BindingLayout>();
            ret->manager_ = this;
            ret->desc = desc;
            ret->rhiLayout_ = device_->CreateBindingLayout(rhiDesc);
            return ret;
        });
}

void BindingGroupManager::_internalRelease(BindingGroup &group)
{
    sync_.OnFrameComplete([group = std::move(group.rhiGroup_)] {});
}

void BindingGroupManager::_internalRelease(BindingGroupLayout &layout)
{
    sync_.OnFrameComplete([layout = std::move(layout.rhiLayout_)] {});
}

void BindingGroupManager::_internalRelease(BindingLayout &layout)
{
    sync_.OnFrameComplete([layout = std::move(layout.rhiLayout_)] {});
}

ConstantBufferManagerInterface *BindingGroupManager::_internalGetDefaultConstantBufferManager()
{
    return defaultConstantBufferManager_;
}

RTRC_END