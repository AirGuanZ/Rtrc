#include <RHI/DirectX12/Context/Device.h>
#include <RHI/DirectX12/Pipeline/BindingGroup.h>
#include <RHI/DirectX12/RayTracing/Tlas.h>
#include <RHI/DirectX12/Resource/BufferView.h>
#include <RHI/DirectX12/Resource/Sampler.h>
#include <RHI/DirectX12/Resource/TextureView.h>

RTRC_RHI_D3D12_BEGIN

DirectX12BindingGroup::DirectX12BindingGroup(
    DirectX12Device                  *device,
    OPtr<DirectX12BindingGroupLayout> layout,
    std::vector<DescriptorTable>      tables,
    uint32_t                          variableArraySize)
    : device_           (device)
    , layout_           (std::move(layout))
    , tables_           (std::move(tables))
    , variableArraySize_(variableArraySize)
{
    
}

DirectX12BindingGroup::~DirectX12BindingGroup()
{
    device_->_internalFreeBindingGroup(*this);
}

const BindingGroupLayout *DirectX12BindingGroup::GetLayout() const
{
    return layout_.Get();
}

uint32_t DirectX12BindingGroup::GetVariableArraySize() const
{
    return variableArraySize_;
}

void DirectX12BindingGroup::ModifyMember(int index, int arrayElem, const BufferSrv *bufferSrv)
{
    assert(GetBindingType(index) == BindingType::Buffer || 
           GetBindingType(index) == BindingType::StructuredBuffer);
    auto d3dBufferSrv = static_cast<const DirectX12BufferSrv *>(bufferSrv);
    ModifyMemberImpl(
        index, arrayElem, d3dBufferSrv->_internalGetDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DirectX12BindingGroup::ModifyMember(int index, int arrayElem, const BufferUav *bufferUav)
{
    assert(GetBindingType(index) == BindingType::RWBuffer || 
           GetBindingType(index) == BindingType::RWStructuredBuffer);
    auto d3dBufferUav = static_cast<const DirectX12BufferUav *>(bufferUav);
    ModifyMemberImpl(
        index, arrayElem, d3dBufferUav->_internalGetDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DirectX12BindingGroup::ModifyMember(int index, int arrayElem, const TextureSrv *textureSrv)
{
    assert(GetBindingType(index) == BindingType::Texture);
    auto d3dTextureSrv = static_cast<const DirectX12TextureSrv *>(textureSrv);
    ModifyMemberImpl(
        index, arrayElem, d3dTextureSrv->_internalGetDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DirectX12BindingGroup::ModifyMember(int index, int arrayElem, const TextureUav *textureUav)
{
    assert(GetBindingType(index) == BindingType::RWTexture);
    auto d3dTextureUav = static_cast<const DirectX12TextureUav *>(textureUav);
    ModifyMemberImpl(
        index, arrayElem, d3dTextureUav->_internalGetDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DirectX12BindingGroup::ModifyMember(int index, int arrayElem, const  Sampler *sampler)
{
    assert(GetBindingType(index) == BindingType::Sampler);
    auto d3dSampler = static_cast<const DirectX12Sampler *>(sampler);
    ModifyMemberImpl(
        index, arrayElem, d3dSampler->_internalGetNativeSampler(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

void DirectX12BindingGroup::ModifyMember(int index, int arrayElem, const ConstantBufferUpdate &cbuffer)
{
    assert(GetBindingType(index) == BindingType::ConstantBuffer);
    auto d3dBuffer = static_cast<const DirectX12Buffer *>(cbuffer.buffer);

    D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc;
    viewDesc.BufferLocation = d3dBuffer->GetDeviceAddress().address + cbuffer.offset;
    viewDesc.SizeInBytes = UpAlignTo<UINT>(static_cast<UINT>(cbuffer.range), 256);
    assert(cbuffer.offset + viewDesc.SizeInBytes <= cbuffer.buffer->GetDesc().size);

    const Helper::D3D12BindingAssignment &binding = layout_->_internalGetD3D12Desc().bindings[index];
    const DescriptorTable &table = tables_[binding.tableIndex];
    const auto tableStart = table.cpuHandle;
    const auto dst = device_->_internalOffsetDescriptorHandle(
        tableStart, binding.offsetInTable + arrayElem, false);
    
    device_->_internalGetNativeDevice()->CreateConstantBufferView(&viewDesc, dst);
}

void DirectX12BindingGroup::ModifyMember(int index, int arrayElem, const Tlas *tlas)
{
    assert(GetBindingType(index) == BindingType::AccelerationStructure);
    auto d3dTlas = static_cast<const DirectX12Tlas *>(tlas);
    ModifyMemberImpl(
        index, arrayElem, d3dTlas->_internalGetSrv(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DirectX12BindingGroup::ModifyMemberImpl(
    int index, int arrayElem, const D3D12_CPU_DESCRIPTOR_HANDLE &src, D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
    const Helper::D3D12BindingAssignment &binding = layout_->_internalGetD3D12Desc().bindings[index];
    const DescriptorTable &table = tables_[binding.tableIndex];
    const auto tableStart = table.cpuHandle;
    const auto dst = device_->_internalOffsetDescriptorHandle(
        tableStart, binding.offsetInTable + arrayElem, heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    device_->_internalGetNativeDevice()->CopyDescriptorsSimple(1, dst, src, heapType);
}

BindingType DirectX12BindingGroup::GetBindingType(int index) const
{
    return layout_->GetDesc().bindings[index].type;
}

RTRC_RHI_D3D12_END
