#pragma once

#include <any>
#include <map>

#include <Rtrc/Graphics/Device/DeviceSynchronizer.h>
#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/Sampler.h>
#include <Rtrc/Graphics/Device/Texture.h>
#include <Rtrc/Utility/ObjectCache.h>

RTRC_BEGIN

class BindingGroupLayout;
class BindingGroup;
class BindingLayout;
class Sampler;
class SubBuffer;

class BindingGroup : public Uncopyable
{
public:

    ~BindingGroup();

    const RC<const BindingGroupLayout> &GetLayout() const;

    void Set(int slot, RC<Buffer>    cbuffer, size_t offset, size_t size);
    void Set(int slot, RC<SubBuffer> cbuffer);
    void Set(int slot, RC<Sampler>   sampler);

    template<typename T>
        requires TypeList<BufferSRV, BufferUAV, TextureSRV, TextureUAV, RC<Texture>>::Contains<std::remove_cvref_t<T>>
    void Set(int slot, T &&object);

    template<typename T>
    void Set(std::string_view name, T &&object);

    template<typename T>
    void Set(const T &value); // See BindingGroupDSL.h

    const RHI::BindingGroupPtr &GetRHIObject() const;

private:

    friend class BindingGroupManager;
    friend class BindingGroupLayout;

    BindingGroupManager *manager_ = nullptr;
    RC<const BindingGroupLayout> layout_;
    RHI::BindingGroupPtr rhiGroup_;
    std::vector<std::any> boundObjects_;
};

class BindingGroupLayout :
    public InObjectCache,
    public std::enable_shared_from_this<BindingGroupLayout>
{
public:

    struct BindingDesc
    {
        std::string              name;
        RHI::BindingType         type;
        RHI::ShaderStageFlag     stages;
        std::optional<uint32_t>  arraySize;
        std::vector<RC<Sampler>> immutableSamplers;

        auto operator<=>(const BindingDesc &) const = default;
        bool operator==(const BindingDesc &) const = default;
    };

    struct Desc
    {
        std::vector<BindingDesc> bindings;

        auto operator<=>(const Desc &) const = default;
        bool operator==(const Desc &) const = default;
    };

    ~BindingGroupLayout();

    int GetBindingSlotByName(std::string_view name) const;

    const RHI::BindingGroupLayoutPtr &GetRHIObject() const;

    RC<BindingGroup> CreateBindingGroup() const;

private:

    friend class BindingGroupManager;

    BindingGroupManager *manager_ = nullptr;
    std::map<std::string, int, std::less<>> nameToSlot_;
    RHI::BindingGroupLayoutPtr rhiLayout_;
};

class BindingLayout : public InObjectCache
{
public:

    struct Desc
    {
        std::vector<RC<BindingGroupLayout>> groupLayouts;
        auto operator<=>(const Desc &) const = default;
    };

    ~BindingLayout();

    const RHI::BindingLayoutPtr &GetRHIObject() const;

private:

    friend class BindingGroupManager;

    BindingGroupManager *manager_ = nullptr;
    Desc desc; // group layouts will not be released before the parent binding layout is released
    RHI::BindingLayoutPtr rhiLayout_;
};

class BindingGroupManager : public Uncopyable
{
public:

    BindingGroupManager(
        RHI::DevicePtr                  device,
        DeviceSynchronizer             &sync,
        ConstantBufferManagerInterface *defaultConstantBufferManager);

    RC<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayout::Desc &desc);
    RC<BindingGroup>       CreateBindingGroup(RC<const BindingGroupLayout> groupLayout);
    RC<BindingLayout>      CreateBindingLayout(const BindingLayout::Desc &desc);

    void _internalRelease(BindingGroup &group);
    void _internalRelease(BindingGroupLayout &layout);
    void _internalRelease(BindingLayout &layout);
    ConstantBufferManagerInterface *_internalGetDefaultConstantBufferManager();

private:

    RHI::DevicePtr device_;
    DeviceSynchronizer &sync_;

    ConstantBufferManagerInterface *defaultConstantBufferManager_;

    ObjectCache<BindingGroupLayout::Desc, BindingGroupLayout, true, false> groupLayoutCache_;
    ObjectCache<BindingLayout::Desc, BindingLayout, true, false> layoutCache_;
};

inline BindingGroup::~BindingGroup()
{
    if(manager_)
    {
        manager_->_internalRelease(*this);
    }
}

inline const RC<const BindingGroupLayout> &BindingGroup::GetLayout() const
{
    return layout_;
}

inline void BindingGroup::Set(int slot, RC<Buffer> cbuffer, size_t offset, size_t size)
{
    rhiGroup_->ModifyMember(slot, cbuffer->GetRHIObject(), offset, size);
    boundObjects_[slot] = std::move(cbuffer);
}

inline void BindingGroup::Set(int slot, RC<SubBuffer> cbuffer)
{
    rhiGroup_->ModifyMember(
        slot, cbuffer->GetFullBuffer()->GetRHIObject(), cbuffer->GetSubBufferOffset(), cbuffer->GetSubBufferSize());
    boundObjects_[slot] = std::move(cbuffer);
}

inline void BindingGroup::Set(int slot, RC<Sampler> sampler)
{
    rhiGroup_->ModifyMember(slot, sampler->GetRHIObject());
    boundObjects_[slot] = std::move(sampler);
}

template<typename T>
    requires TypeList<BufferSRV, BufferUAV, TextureSRV, TextureUAV, RC<Texture>>::Contains<std::remove_cvref_t<T>>
void BindingGroup::Set(int slot, T &&object)
{
    if constexpr(std::is_same_v<std::remove_cvref_t<T>, RC<Texture>>)
    {
        const RHI::BindingType type = layout_->GetRHIObject()->GetDesc().bindings[slot].type;
        switch(type)
        {
        case RHI::BindingType::Texture2D:
        case RHI::BindingType::Texture3D:
            this->Set(slot, object->CreateSRV(0, 0, 0));
            break;
        case RHI::BindingType::Texture2DArray:
        case RHI::BindingType::Texture3DArray:
            this->Set(slot, object->CreateSRV(0, 0, 0, 0));
            break;
        case RHI::BindingType::RWTexture2D:
        case RHI::BindingType::RWTexture3D:
            this->Set(slot, object->CreateUAV(0, 0));
            break;
        case RHI::BindingType::RWTexture2DArray:
        case RHI::BindingType::RWTexture3DArray:
            this->Set(slot, object->CreateUAV(0, 0, 0));
            break;
        default:
            throw Exception(fmt::format(
                "BindingGroup::Set: cannot bind texture to slot {} (type = {})", slot, GetBindingTypeName(type)));
        }
    }
    else
    {
        rhiGroup_->ModifyMember(slot, object.GetRHIObject());
        boundObjects_[slot] = std::move(object);
    }
}

template<typename T>
void BindingGroup::Set(std::string_view name, T &&object)
{
    const int slot = layout_->GetBindingSlotByName(name);
    this->Set(slot, std::forward<T>(object));
}

inline const RHI::BindingGroupPtr &BindingGroup::GetRHIObject() const
{
    return rhiGroup_;
}

inline BindingGroupLayout::~BindingGroupLayout()
{
    if(manager_)
    {
        manager_->_internalRelease(*this);
    }
}

inline int BindingGroupLayout::GetBindingSlotByName(std::string_view name) const
{
    auto it = nameToSlot_.find(name);
    return it == nameToSlot_.end() ? -1 : it->second;
}

inline RC<BindingGroup> BindingGroupLayout::CreateBindingGroup() const
{
    return manager_->CreateBindingGroup(shared_from_this());
}

inline const RHI::BindingGroupLayoutPtr &BindingGroupLayout::GetRHIObject() const
{
    return rhiLayout_;
}

inline BindingLayout::~BindingLayout()
{
    if(manager_)
    {
        manager_->_internalRelease(*this);
    }
}
inline const RHI::BindingLayoutPtr &BindingLayout::GetRHIObject() const
{
    return rhiLayout_;
}


RTRC_END
