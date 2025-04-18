#include <Rtrc/Graphics/Device/BindingGroup.h>

RTRC_BEGIN

void DumpBindingGroupLayoutDesc(const RHI::BindingGroupLayoutDesc &desc)
{
    LogError("{{");
    for(size_t i = 0; i < desc.bindings.size(); ++i)
    {
        auto &binding = desc.bindings[i];
        if(!binding.immutableSamplers.empty())
        {
            LogError("    Immutable samplers");
            LogError("    {{");
            for(const RHI::SamplerRPtr &sampler : binding.immutableSamplers)
            {
                const RHI::SamplerDesc &samplerDesc = sampler->GetDesc();
                LogError("        Hash = {}", samplerDesc.Hash());
            }
            LogError("    }}");
            continue;
        }

        std::string line = std::format(
            "[{}] {}", GetShaderStageFlagsName(binding.stages), GetBindingTypeName(binding.type));
        if(binding.arraySize)
        {
            line += std::format(" [{}]", *binding.arraySize);
        }
        if(binding.bindless)
        {
            line += " bindless";
        }
        if(i + 1 == desc.bindings.size() && desc.variableArraySize)
        {
            line += " varSize";
        }
        LogError("    {}", line);
    }
    LogError("}}");
}

BindingGroupManager::BindingGroupManager(
    RHI::DeviceOPtr device, DeviceSynchronizer &sync, ConstantBufferManagerInterface *defaultConstantBufferManager)
    : device_(std::move(device)), sync_(&sync), defaultConstantBufferManager_(defaultConstantBufferManager)
{

}

RC<BindingGroupLayout> BindingGroupManager::CreateBindingGroupLayout(const BindingGroupLayout::Desc &desc)
{
    return groupLayoutCache_.GetOrCreate(desc, [&]
        {
            RHI::BindingGroupLayoutDesc rhiDesc;
            for(auto &binding : desc.bindings)
            {
                RHI::BindingDesc rhiBinding;
                rhiBinding.type = binding.type;
                rhiBinding.stages = binding.stages;
                rhiBinding.arraySize = binding.arraySize;
                for(auto &s : binding.immutableSamplers)
                {
                    rhiBinding.immutableSamplers.push_back(s->GetRHIObject());
                }
                rhiBinding.bindless = binding.bindless;
                rhiDesc.bindings.push_back(rhiBinding);
            }
            rhiDesc.variableArraySize = desc.variableArraySize;

            auto ret = MakeRC<BindingGroupLayout>();
            ret->manager_ = this;
            ret->rhiLayout_ = device_->CreateBindingGroupLayout(rhiDesc);
            return ret;
        });
}

RC<BindingGroup> BindingGroupManager::CreateBindingGroup(
    RC<const BindingGroupLayout> groupLayout, int variableBindingCount)
{
    auto ret = MakeRC<BindingGroup>();
    ret->rhiGroup_ = device_->CreateBindingGroup(groupLayout->rhiLayout_, variableBindingCount);
    ret->manager_ = this;
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
        rhiDesc.pushConstantRanges = desc.pushConstantRanges;
        rhiDesc.unboundedAliases = desc.unboundedAliases;
    
        auto ret = MakeRC<BindingLayout>();
        ret->manager_ = this;
        ret->desc = desc;
        ret->rhiLayout_ = device_->CreateBindingLayout(rhiDesc);
        return ret;
    });
}

void BindingGroupManager::CopyBindings(
    const RC<BindingGroup> &dst, uint32_t dstSlot, uint32_t dstArrElem,
    const RC<BindingGroup> &src, uint32_t srcSlot, uint32_t srcArrElem,
    uint32_t count)
{
    device_->CopyBindingGroup(
        dst->GetRHIObject(), dstSlot, dstArrElem,
        src->GetRHIObject(), srcSlot, srcArrElem, count);
}

void BindingGroupManager::_internalRelease(BindingGroup &group)
{
    sync_->OnFrameComplete([group = std::move(group.rhiGroup_)] {});
}

void BindingGroupManager::_internalRelease(BindingGroupLayout &layout)
{
    sync_->OnFrameComplete([layout = std::move(layout.rhiLayout_)] {});
}

void BindingGroupManager::_internalRelease(BindingLayout &layout)
{
    sync_->OnFrameComplete([layout = std::move(layout.rhiLayout_)] {});
}

ConstantBufferManagerInterface *BindingGroupManager::_internalGetDefaultConstantBufferManager()
{
    return defaultConstantBufferManager_;
}

RHI::Device *BindingGroupManager::_internalGetRHIDevice()
{
    return device_;
}

RTRC_END
