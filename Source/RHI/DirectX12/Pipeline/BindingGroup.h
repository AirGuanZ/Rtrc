#pragma once

#include <RHI/DirectX12/Pipeline/BindingGroupLayout.h>
#include <RHI/DirectX12/Context/DescriptorHeap.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12BindingGroup, BindingGroup)
{
public:

#ifdef RTRC_STATIC_RHI
    RTRC_RHI_BINDING_GROUP_COMMON
#endif

    DirectX12BindingGroup(
        DirectX12Device                 *device,
        Ptr<DirectX12BindingGroupLayout> layout,
        std::vector<DescriptorTable>     tables,
        uint32_t                         variableArraySize);

    ~DirectX12BindingGroup() override;

    const BindingGroupLayout *GetLayout() const RTRC_RHI_OVERRIDE;
    uint32_t GetVariableArraySize() const RTRC_RHI_OVERRIDE;

    void ModifyMember(int index, int arrayElem, const BufferSrv            *bufferSrv)  RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, const BufferUav            *bufferUav)  RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, const TextureSrv           *textureSrv) RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, const TextureUav           *textureUav) RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, const Sampler              *sampler)    RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, const ConstantBufferUpdate &cbuffer)    RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, const Tlas                 *tlas)       RTRC_RHI_OVERRIDE;

    Span<DescriptorTable> _internalGetDescriptorTables() const { return tables_; }

private:

    void ModifyMemberImpl(
        int index, int arrayElem, const D3D12_CPU_DESCRIPTOR_HANDLE &src, D3D12_DESCRIPTOR_HEAP_TYPE heapType);

    BindingType GetBindingType(int index) const;

    DirectX12Device                 *device_;
    Ptr<DirectX12BindingGroupLayout> layout_;
    std::vector<DescriptorTable>     tables_;
    uint32_t                         variableArraySize_;
};

RTRC_RHI_D3D12_END
