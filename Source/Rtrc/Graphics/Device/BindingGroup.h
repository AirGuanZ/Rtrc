#pragma once

#include <any>
#include <map>

#include <Rtrc/Graphics/Device/AccelerationStructure.h>
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
class Tlas;

class BindingGroup : public Uncopyable
{
public:

    ~BindingGroup();

    const RC<const BindingGroupLayout> &GetLayout() const;

    void Set(int slot, RC<Buffer>    cbuffer, size_t offset, size_t size);
    void Set(int slot, RC<SubBuffer> cbuffer);
    void Set(int slot, RC<Sampler>   sampler);
    void Set(int slot, RC<Tlas>      tlas);

    void Set(int slot, int arrElem, RC<Buffer>    cbuffer, size_t offset, size_t size);
    void Set(int slot, int arrElem, RC<SubBuffer> cbuffer);
    void Set(int slot, int arrElem, RC<Sampler>   sampler);
    void Set(int slot, int arrElem, RC<Tlas>      tlas);

    template<typename T>
        requires TypeList<BufferSrv, BufferUav, TextureSrv, TextureUav, RC<Texture>>::Contains<std::remove_cvref_t<T>>
    void Set(int slot, T &&object);
    template<typename T>
        requires TypeList<BufferSrv, BufferUav, TextureSrv, TextureUav, RC<Texture>>::Contains<std::remove_cvref_t<T>>
    void Set(int slot, int arrElem, T &&object);

    template<typename T>
    void Set(const T &value); // Impl in BindingGroupDSL.h
    
    const RHI::BindingGroupPtr &GetRHIObject() const;

private:

    friend class BindingGroupManager;
    friend class BindingGroupLayout;

    BindingGroupManager *manager_ = nullptr;
    RC<const BindingGroupLayout> layout_;
    RHI::BindingGroupPtr rhiGroup_;
};

class BindingGroupLayout : public InObjectCache, public std::enable_shared_from_this<BindingGroupLayout>
{
public:

    struct BindingDesc
    {
        RHI::BindingType         type;
        RHI::ShaderStageFlag     stages;
        std::optional<uint32_t>  arraySize;
        std::vector<RC<Sampler>> immutableSamplers;
        bool                     bindless = false;

        auto operator<=>(const BindingDesc &) const = default;
        bool operator==(const BindingDesc &) const = default;
    };

    struct Desc
    {
        std::vector<BindingDesc> bindings;
        bool variableArraySize = false;

        auto operator<=>(const Desc &) const = default;
        bool operator==(const Desc &) const = default;
    };

    ~BindingGroupLayout();

    const RHI::BindingGroupLayoutPtr &GetRHIObject() const;

    RC<BindingGroup> CreateBindingGroup(int variableBindingCount = 0) const;

private:

    friend class BindingGroupManager;

    BindingGroupManager *manager_ = nullptr;
    RHI::BindingGroupLayoutPtr rhiLayout_;
};

class BindingLayout : public InObjectCache
{
public:

    struct Desc
    {
        std::vector<RC<BindingGroupLayout>> groupLayouts;
        std::vector<RHI::PushConstantRange> pushConstantRanges;
        auto operator<=>(const Desc &) const = default;
    };

    ~BindingLayout();

    const RHI::BindingLayoutPtr &GetRHIObject() const;

private:

    friend class BindingGroupManager;

    BindingGroupManager *manager_ = nullptr;
    Desc desc; // Group layouts will not be released before the parent binding layout is released
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
    RC<BindingGroup>       CreateBindingGroup(RC<const BindingGroupLayout> groupLayout, int variableBindingCount = 0);
    RC<BindingLayout>      CreateBindingLayout(const BindingLayout::Desc &desc);

    void _internalRelease(BindingGroup &group);
    void _internalRelease(BindingGroupLayout &layout);
    void _internalRelease(BindingLayout &layout);
    ConstantBufferManagerInterface *_internalGetDefaultConstantBufferManager();
    const RHI::DevicePtr &_internalGetRHIDevice();

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
    Set(slot, 0, std::move(cbuffer), offset, size);
}

inline void BindingGroup::Set(int slot, RC<SubBuffer> cbuffer)
{
    Set(slot, 0, std::move(cbuffer));
}

inline void BindingGroup::Set(int slot, RC<Sampler> sampler)
{
    Set(slot, 0, std::move(sampler));
}

inline void BindingGroup::Set(int slot, RC<Tlas> tlas)
{
    Set(slot, 0, std::move(tlas));
}

inline void BindingGroup::Set(int slot, int arrElem, RC<Buffer> cbuffer, size_t offset, size_t size)
{
    rhiGroup_->ModifyMember(slot, arrElem, RHI::ConstantBufferUpdate{ cbuffer->GetRHIObject().Get(), offset, size });
}

inline void BindingGroup::Set(int slot, int arrElem, RC<SubBuffer> cbuffer)
{
    const RHI::ConstantBufferUpdate update =
    {
        cbuffer->GetFullBuffer()->GetRHIObject().Get(),
        cbuffer->GetSubBufferOffset(),
        cbuffer->GetSubBufferSize()
    };
    rhiGroup_->ModifyMember(slot, arrElem, update);
}

inline void BindingGroup::Set(int slot, int arrElem, RC<Sampler> sampler)
{
    rhiGroup_->ModifyMember(slot, arrElem, sampler->GetRHIObject());
}

inline void BindingGroup::Set(int slot, int arrElem, RC<Tlas> tlas)
{
    rhiGroup_->ModifyMember(slot, arrElem, tlas->GetRHIObject());
}

template<typename T>
    requires TypeList<BufferSrv, BufferUav, TextureSrv, TextureUav, RC<Texture>>::Contains<std::remove_cvref_t<T>>
void BindingGroup::Set(int slot, T &&object)
{
    this->Set(slot, 0, std::forward<T>(object));
}

template<typename T>
    requires TypeList<BufferSrv, BufferUav, TextureSrv, TextureUav, RC<Texture>>::Contains<std::remove_cvref_t<T>>
void BindingGroup::Set(int slot, int arrElem, T &&object)
{
    if constexpr(std::is_same_v<std::remove_cvref_t<T>, RC<Texture>>)
    {
        switch(const RHI::BindingType type = layout_->GetRHIObject()->GetDesc().bindings[slot].type)
        {
        case RHI::BindingType::Texture2D:
        case RHI::BindingType::Texture3D:
            this->Set(slot, arrElem, object->CreateSrv(0, 0, 0));
            break;
        case RHI::BindingType::Texture2DArray:
        case RHI::BindingType::Texture3DArray:
            this->Set(slot, arrElem, object->CreateSrv(0, 0, 0, 0));
            break;
        case RHI::BindingType::RWTexture2D:
        case RHI::BindingType::RWTexture3D:
            this->Set(slot, arrElem, object->CreateUav(0, 0));
            break;
        case RHI::BindingType::RWTexture2DArray:
        case RHI::BindingType::RWTexture3DArray:
            this->Set(slot, arrElem, object->CreateUav(0, 0, 0));
            break;
        default:
            throw Exception(fmt::format(
                "BindingGroup::Set: cannot bind texture to slot {}[{}] (type = {})",
                slot, arrElem, GetBindingTypeName(type)));
        }
    }
    else
    {
        rhiGroup_->ModifyMember(slot, arrElem, object.GetRHIObject());
    }
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

inline RC<BindingGroup> BindingGroupLayout::CreateBindingGroup(int variableBindingCount) const
{
    return manager_->CreateBindingGroup(shared_from_this(), variableBindingCount);
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
