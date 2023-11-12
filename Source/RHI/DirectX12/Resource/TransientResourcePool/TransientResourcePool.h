#pragma once

#include <RHI/DirectX12/Resource/TransientResourcePool/MemoryBlockPool.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12TransientResourcePool, TransientResourcePool)
{
public:

    DirectX12TransientResourcePool(DirectX12Device *device, const TransientResourcePoolDesc &desc);

    int StartHostSynchronizationSession() RTRC_RHI_OVERRIDE;

    void NotifyExternalHostSynchronization(int session) RTRC_RHI_OVERRIDE;

    void Allocate(
        MutableSpan<TransientResourceDeclaration>  resources,
        std::vector<AliasedTransientResourcePair> &aliasRelation) RTRC_RHI_OVERRIDE;

private:

    using Category = TransientResourcePoolDetail::MemoryBlockPool::Category;
    using Alignment = TransientResourcePoolDetail::MemoryBlockPool::Alignment;

    Category GetTextureCategory(const TextureDesc &desc) const;
    Alignment GetTextureHeapAlignment(const TextureDesc &desc) const;

    DirectX12Device *device_;
    TransientResourcePoolDetail::MemoryBlockPool memoryBlocks_;

    D3D12_RESOURCE_HEAP_TIER resourceHeapTier_;
};

RTRC_RHI_D3D12_END
