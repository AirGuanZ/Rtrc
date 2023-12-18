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
        MutSpan<TransientResourceDeclaration> resources,
        std::vector<AliasedTransientResourcePair> &aliasRelation) RTRC_RHI_OVERRIDE;

private:

    using Category = TransientResourcePoolDetail::MemoryBlockPool::Category;
    using HeapAlignment = TransientResourcePoolDetail::MemoryBlockPool::HeapAlignment;

    struct ResourceSlot
    {
        RHIObjectPtr ptr;
        int sessionIndex;
    };

    Category GetTextureCategory(const TextureDesc &desc) const;
    HeapAlignment GetTextureHeapAlignment(const TextureDesc &desc) const;

    DirectX12Device *device_;
    TransientResourcePoolDetail::MemoryBlockPool memoryBlocks_;

    int currentSession_;
    std::vector<ResourceSlot> resources_;

    D3D12_RESOURCE_HEAP_TIER resourceHeapTier_;
};

RTRC_RHI_D3D12_END
