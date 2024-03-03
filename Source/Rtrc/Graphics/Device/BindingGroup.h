#pragma once

#include <map>

#include <Rtrc/Graphics/Device/AccelerationStructure.h>
#include <Rtrc/Graphics/Device/DeviceSynchronizer.h>
#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/Sampler.h>
#include <Rtrc/Graphics/Device/Texture.h>
#include <Rtrc/Core/Container/Cache/SharedObjectCache.h>

RTRC_BEGIN

class BindingGroupLayout;
class BindingGroup;
class BindingLayout;
class Sampler;
class SubBuffer;
class Tlas;

void DumpBindingGroupLayoutDesc(const RHI::BindingGroupLayoutDesc &desc);

class BindingGroup : public Uncopyable
{
public:

    ~BindingGroup();

    const RC<const BindingGroupLayout> &GetLayout() const;
    uint32_t GetVariableArraySize() const;

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
    
    const RHI::BindingGroupUPtr &GetRHIObject() const;

private:

    friend class BindingGroupManager;
    friend class BindingGroupLayout;

    BindingGroupManager         *manager_ = nullptr;
    RC<const BindingGroupLayout> layout_;
    RHI::BindingGroupUPtr        rhiGroup_;
};

class BindingGroupLayout : public InSharedObjectCache, public std::enable_shared_from_this<BindingGroupLayout>
{
public:

    struct BindingDesc
    {
        RHI::BindingType         type;
        RHI::ShaderStageFlags    stages;
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

    ~BindingGroupLayout() override;

    const RHI::BindingGroupLayoutUPtr &GetRHIObject() const;

    RC<BindingGroup> CreateBindingGroup(int variableBindingCount = 0) const;

private:

    friend class BindingGroupManager;

    BindingGroupManager *manager_ = nullptr;
    RHI::BindingGroupLayoutUPtr rhiLayout_;
};

class BindingLayout : public InSharedObjectCache
{
public:

    struct Desc
    {
        std::vector<RC<BindingGroupLayout>> groupLayouts;
        std::vector<RHI::PushConstantRange> pushConstantRanges;
        std::vector<RHI::UnboundedBindingArrayAliasing> unboundedAliases;
        auto operator<=>(const Desc &) const = default;
    };

    ~BindingLayout() override;

    const RHI::BindingLayoutUPtr &GetRHIObject() const;

private:

    friend class BindingGroupManager;

    BindingGroupManager *manager_ = nullptr;
    Desc desc; // Group layouts will not be released before the parent binding layout is released
    RHI::BindingLayoutUPtr rhiLayout_;
};

class BindingGroupManager : public Uncopyable
{
public:

    BindingGroupManager(
        RHI::DeviceOPtr                 device,
        DeviceSynchronizer             &sync,
        ConstantBufferManagerInterface *defaultConstantBufferManager);

    RC<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayout::Desc &desc);
    RC<BindingGroup>       CreateBindingGroup(RC<const BindingGroupLayout> groupLayout, int variableBindingCount = 0);
    RC<BindingLayout>      CreateBindingLayout(const BindingLayout::Desc &desc);

    void CopyBindings(
        const RC<BindingGroup> &dst, uint32_t dstSlot, uint32_t dstArrElem,
        const RC<BindingGroup> &src, uint32_t srcSlot, uint32_t srcArrElem,
        uint32_t count);

    void _internalRelease(BindingGroup &group);
    void _internalRelease(BindingGroupLayout &layout);
    void _internalRelease(BindingLayout &layout);
    ConstantBufferManagerInterface *_internalGetDefaultConstantBufferManager();
    RHI::Device *_internalGetRHIDevice();

private:

    RHI::DeviceOPtr device_;
    DeviceSynchronizer *sync_;

    ConstantBufferManagerInterface *defaultConstantBufferManager_;

    SharedObjectCache<BindingGroupLayout::Desc, BindingGroupLayout, true, false> groupLayoutCache_;
    SharedObjectCache<BindingLayout::Desc, BindingLayout, true, false> layoutCache_;
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

inline uint32_t BindingGroup::GetVariableArraySize() const
{
    return rhiGroup_->GetVariableArraySize();
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
        cbuffer->GetFullBufferRHIObject().Get(),
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
        case RHI::BindingType::Texture:
            this->Set(slot, arrElem, object->GetSrv(0, 0, 0));
            break;
        case RHI::BindingType::RWTexture:
            this->Set(slot, arrElem, object->GetUav(0, 0));
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

inline const RHI::BindingGroupUPtr &BindingGroup::GetRHIObject() const
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

inline const RHI::BindingGroupLayoutUPtr &BindingGroupLayout::GetRHIObject() const
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

inline const RHI::BindingLayoutUPtr &BindingLayout::GetRHIObject() const
{
    return rhiLayout_;
}

RTRC_END
