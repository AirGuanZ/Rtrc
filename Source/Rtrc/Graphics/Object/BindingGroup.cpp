#include <shared_mutex>

#include <Rtrc/Graphics/Object/BindingGroup.h>
#include <Rtrc/Graphics/Object/Buffer.h>
#include <Rtrc/Graphics/Object/ConstantBuffer.h>
#include <Rtrc/Graphics/Object/Sampler.h>
#include <Rtrc/Graphics/Object/Texture.h>

RTRC_BEGIN

BindingGroup::~BindingGroup()
{
    if(manager_)
    {
        manager_->_rtrcReleaseInternal(*this);
    }
}

const RC<const BindingGroupLayout> &BindingGroup::GetLayout() const
{
    return layout_;
}

void BindingGroup::Set(int slot, RC<ConstantBuffer> cbuffer)
{
    rhiGroup_->ModifyMember(slot, cbuffer->GetBuffer(), cbuffer->GetOffset(), cbuffer->GetSize());
    boundObjects_[slot] = std::move(cbuffer);
}

void BindingGroup::Set(int slot, RC<Sampler> sampler)
{
    rhiGroup_->ModifyMember(slot, sampler->GetRHIObject());
    boundObjects_[slot] = std::move(sampler);
}

void BindingGroup::Set(int slot, const BufferSRV &srv)
{
    rhiGroup_->ModifyMember(slot, srv.rhiSRV);
    boundObjects_[slot] = srv;
}

void BindingGroup::Set(int slot, const BufferUAV &uav)
{
    rhiGroup_->ModifyMember(slot, uav.rhiUAV);
    boundObjects_[slot] = uav;
}

void BindingGroup::Set(int slot, const TextureSRV &srv)
{
    rhiGroup_->ModifyMember(slot, srv.rhiSRV);
    boundObjects_[slot] = srv;
}

void BindingGroup::Set(int slot, const TextureUAV &uav)
{
    rhiGroup_->ModifyMember(slot, uav.rhiUAV);
    boundObjects_[slot] = uav;
}

void BindingGroup::Set(int slot, const RC<Texture> &tex)
{
    const RHI::BindingType type =  layout_->GetRHIObject()->GetDesc().bindings[slot].type;
    switch(type)
    {
    case RHI::BindingType::Texture2D:
    case RHI::BindingType::Texture3D:
        Set(slot, tex->GetSRV());
        break;
    case RHI::BindingType::Texture2DArray:
    case RHI::BindingType::Texture3DArray:
        Set(slot, tex->GetSRV({ .isArray = true }));
        break;
    case RHI::BindingType::RWTexture2D:
    case RHI::BindingType::RWTexture3D:
        Set(slot, tex->GetUAV());
        break;
    case RHI::BindingType::RWTexture2DArray:
    case RHI::BindingType::RWTexture3DArray:
        Set(slot, tex->GetUAV({ .isArray = true }));
        break;
    default:
        throw Exception(fmt::format(
            "BindingGroup::Set: cannot bind texture to slot {} (type = {})", slot, GetBindingTypeName(type)));
    }
}

void BindingGroup::Set(std::string_view name, RC<ConstantBuffer> cbuffer)
{
    const int slot = layout_->GetBindingSlotByName(name);
    Set(slot, std::move(cbuffer));
}

void BindingGroup::Set(std::string_view name, RC<Sampler> sampler)
{
    const int slot = layout_->GetBindingSlotByName(name);
    Set(slot, std::move(sampler));
}

void BindingGroup::Set(std::string_view name, const BufferSRV &srv)
{
    const int slot = layout_->GetBindingSlotByName(name);
    Set(slot, srv);
}

void BindingGroup::Set(std::string_view name, const BufferUAV &uav)
{
    const int slot = layout_->GetBindingSlotByName(name);
    Set(slot, uav);
}

void BindingGroup::Set(std::string_view name, const TextureSRV &srv)
{
    const int slot = layout_->GetBindingSlotByName(name);
    Set(slot, srv);
}

void BindingGroup::Set(std::string_view name, const TextureUAV &uav)
{
    const int slot = layout_->GetBindingSlotByName(name);
    Set(slot, uav);
}

void BindingGroup::Set(std::string_view name, const RC<Texture> &tex)
{
    const int slot = layout_->GetBindingSlotByName(name);
    Set(slot, tex);
}

const RHI::BindingGroupPtr &BindingGroup::GetRHIObject()
{
    return rhiGroup_;
}

BindingGroupLayout::~BindingGroupLayout()
{
    if(manager_)
    {
        manager_->_rtrcReleaseInternal(*this);
    }
}

int BindingGroupLayout::GetBindingSlotByName(std::string_view name) const
{
    auto it = record_->nameToSlot.find(name);
    if(it == record_->nameToSlot.end())
    {
        throw Exception(fmt::format(
            "Binding slot {} is not found in given binding group layout", name));
    }
    return it->second;
}

const RHI::BindingGroupLayoutPtr &BindingGroupLayout::GetRHIObject() const
{
    return record_->rhiLayout;
}

RC<BindingGroup> BindingGroupLayout::CreateBindingGroup() const
{
    return manager_->CreateBindingGroup(shared_from_this());
}

BindingLayout::~BindingLayout()
{
    if(manager_)
    {
        manager_->_rtrcReleaseInternal(*this);
    }
}

const RHI::BindingLayoutPtr &BindingLayout::GetRHIObject() const
{
    return record_->rhiLayout;
}

BindingLayoutManager::BindingLayoutManager(HostSynchronizer &hostSync, RHI::DevicePtr device)
    : device_(std::move(device)), groupLayoutReleaser_(hostSync), layoutReleaser_(hostSync), groupReleaser_(hostSync)
{
    groupLayoutReleaser_.SetPreNewBatchCallback([this]
    {
        std::vector<ReleaseBindingGroupLayoutData> pendingLayoutReleaseData;
        {
            std::lock_guard lock(pendingGroupLayoutReleaseDataMutex_);
            pendingLayoutReleaseData.swap(pendingGroupLayoutReleaseData_);
        }
        for(auto &l : pendingLayoutReleaseData)
            groupLayoutReleaser_.AddToCurrentBatch(l);
    });

    using GroupLayoutReleaseDataList = BatchReleaseHelper<ReleaseBindingGroupLayoutData>::DataList;
    groupLayoutReleaser_.SetReleaseCallback([this](int, GroupLayoutReleaseDataList &dataList)
    {
        std::unique_lock lock(groupLayoutCacheMutex_);
        for(auto &d : dataList)
        {
            if(auto it = groupLayoutCache_.find(d.desc); it != groupLayoutCache_.end())
            {
                if(it->second.expired())
                {
                    groupLayoutCache_.erase(it);
                }
            }
        }
    });

    layoutReleaser_.SetPreNewBatchCallback([this]
    {
        std::vector<ReleaseBindingLayoutData> pendingLayoutReleaseData;
        {
            std::lock_guard lock(pendingLayoutReleaseDataMutex_);
            pendingLayoutReleaseData.swap(pendingLayoutReleaseData_);
        }
        for(auto &l : pendingLayoutReleaseData)
            layoutReleaser_.AddToCurrentBatch(l);
    });

    using LayoutReleaseDataList = BatchReleaseHelper<ReleaseBindingLayoutData>::DataList;
    layoutReleaser_.SetReleaseCallback([this](int, LayoutReleaseDataList &dataList)
    {
        std::unique_lock lock(layoutCacheMutex_);
        for(auto &d : dataList)
        {
            if(auto it = layoutCache_.find(d.desc); it != layoutCache_.end())
            {
                if(it->second.expired())
                {
                    layoutCache_.erase(it);
                }
            }
        }
    });

    groupReleaser_.SetPreNewBatchCallback([this]
    {
        std::vector<ReleaseBindingGroupData> pendingGroupReleaseData;
        {
            std::lock_guard lock(pendingGroupReleaseDataMutex_);
            pendingGroupReleaseData.swap(pendingGroupReleaseData_);
        }
        for(auto &g : pendingGroupReleaseData)
            groupReleaser_.AddToCurrentBatch(g);
    });
}

RC<BindingGroupLayout> BindingLayoutManager::CreateBindingGroupLayout(const RHI::BindingGroupLayoutDesc &desc)
{
    auto ret = MakeRC<BindingGroupLayout>();
    ret->manager_ = this;

    {
        std::shared_lock lock(groupLayoutCacheMutex_);
        if(auto it = groupLayoutCache_.find(desc); it != groupLayoutCache_.end())
        {
            auto p = it->second.lock();
            if(p)
            {
                ret->record_ = std::move(p);
                return ret;
            }
        }
    }

    std::unique_lock lock(groupLayoutCacheMutex_);
    if(auto it = groupLayoutCache_.find(desc); it != groupLayoutCache_.end())
    {
        auto p = it->second.lock();
        if(p)
        {
            ret->record_ = std::move(p);
            return ret;
        }
    }

    auto record = MakeRC<BindingGroupLayoutRecord>();
    record->rhiLayout = device_->CreateBindingGroupLayout(desc);
    for(size_t slot = 0; slot < desc.bindings.size(); ++slot)
    {
        auto &b = desc.bindings[slot];
        record->nameToSlot[b.name] = static_cast<int>(slot);
    }
    groupLayoutCache_.insert({ desc, record });

    ret->record_ = std::move(record);
    return ret;
}

RC<BindingGroup> BindingLayoutManager::CreateBindingGroup(RC<const BindingGroupLayout> layout)
{
    auto rhiGroup = device_->CreateBindingGroup(layout->record_->rhiLayout);
    auto ret = MakeRC<BindingGroup>();
    ret->manager_ = this;
    ret->layout_ = std::move(layout);
    ret->rhiGroup_ = std::move(rhiGroup);
    ret->boundObjects_.resize(ret->layout_->record_->rhiLayout->GetDesc().bindings.size());
    return ret;
}

RC<BindingLayout> BindingLayoutManager::CreateBindingLayout(const BindingLayoutDesc &desc)
{
    auto ret = MakeRC<BindingLayout>();
    ret->manager_ = this;

    {
        std::shared_lock lock(layoutCacheMutex_);
        if(auto it = layoutCache_.find(desc); it != layoutCache_.end())
        {
            auto p = it->second.lock();
            if(p)
            {
                ret->record_ = std::move(p);
                return ret;
            }
        }
    }

    std::unique_lock lock(layoutCacheMutex_);
    if(auto it = layoutCache_.find(desc); it != layoutCache_.end())
    {
        auto p = it->second.lock();
        if(p)
        {
            ret->record_ = std::move(p);
            return ret;
        }
    }

    auto record = MakeRC<BindingLayoutRecord>();
    record->desc = desc;
    RHI::BindingLayoutDesc rhiDesc;
    for(auto &g : desc.groupLayouts)
    {
        rhiDesc.groups.push_back(g->GetRHIObject());
    }
    record->rhiLayout = device_->CreateBindingLayout(rhiDesc);
    layoutCache_.insert({ desc, record });

    ret->record_ = std::move(record);
    return ret;
}

void BindingLayoutManager::_rtrcReleaseInternal(BindingGroup &group)
{
    if(groupReleaser_.GetHostSynchronizer().IsInOwnerThread())
    {
        groupReleaser_.AddToCurrentBatch({ std::move(group.rhiGroup_) });
    }
    else
    {
        std::lock_guard lock(pendingGroupReleaseDataMutex_);
        pendingGroupReleaseData_.push_back({ std::move(group.rhiGroup_) });
    }
}

void BindingLayoutManager::_rtrcReleaseInternal(BindingGroupLayout &groupLayout)
{
    if(groupLayoutReleaser_.GetHostSynchronizer().IsInOwnerThread())
    {
        groupLayoutReleaser_.AddToCurrentBatch({ groupLayout.record_->rhiLayout->GetDesc(), groupLayout.record_->rhiLayout });
    }
    else
    {
        std::lock_guard lock(pendingGroupLayoutReleaseDataMutex_);
        pendingGroupLayoutReleaseData_.push_back({ groupLayout.record_->rhiLayout->GetDesc(), groupLayout.record_->rhiLayout });
    }
}

void BindingLayoutManager::_rtrcReleaseInternal(BindingLayout &layout)
{
    if(layoutReleaser_.GetHostSynchronizer().IsInOwnerThread())
    {
        layoutReleaser_.AddToCurrentBatch({ layout.record_->desc, layout.record_->rhiLayout });
    }
    else
    {
        std::lock_guard lock(pendingLayoutReleaseDataMutex_);
        pendingLayoutReleaseData_.push_back({ layout.record_->desc, layout.record_->rhiLayout });
    }
}

RTRC_END
