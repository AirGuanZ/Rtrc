#include <ranges>

#include <Rtrc/RHI/DirectX12/Context/Device.h>
#include <Rtrc/RHI/DirectX12/Resource/TransientResourcePool/MemoryBlockPool.h>

RTRC_RHI_D3D12_BEGIN

TransientResourcePoolDetail::MemoryBlockPool::MemoryBlockPool(
    DirectX12Device *device, size_t memoryBlockSizeHint)
    : device_(device)
    , memoryBlockSizeHint_(memoryBlockSizeHint)
{

}

const TransientResourcePoolDetail::MemoryBlockPool::MemoryBlock &
    TransientResourcePoolDetail::MemoryBlockPool::GetMemoryBlock(
        QueueOPtr          queue,
        RC<QueueSyncQuery> sync,
        Category           category,
        HeapAlignment      alignment,
        size_t             leastSize)
{
    auto &queueRecord = queueRecords_[queue];

    const int categoryIndex = std::to_underlying(category);
    const int alignmentIndex = std::to_underlying(alignment);
    auto &blocks = queueRecord.availableMemoryBlocks_[categoryIndex][alignmentIndex];

    auto it = std::lower_bound(blocks.begin(), blocks.end(), leastSize, MemoryBlockComp{});
    if(it != blocks.end())
    {
        auto mb = std::move(blocks.extract(it).value());
        mb.queueSync = std::move(sync);
        return *queueRecord.usedMemoryBlocks_[categoryIndex][alignmentIndex].insert(mb);
    }

    if(alignment == HeapAlignment::Regular) // fallback to msaa block, if possible
    {
        const int fallbackAlignmentIndex = std::to_underlying(HeapAlignment::MSAA);
        auto &fallbackBlocks = queueRecord.availableMemoryBlocks_[categoryIndex][fallbackAlignmentIndex];
        it = std::lower_bound(fallbackBlocks.begin(), fallbackBlocks.end(), leastSize, MemoryBlockComp{});
        if(it != fallbackBlocks.end())
        {
            auto mb = std::move(fallbackBlocks.extract(it).value());
            mb.queueSync = std::move(sync);
            return *queueRecord.usedMemoryBlocks_[categoryIndex][fallbackAlignmentIndex].insert(mb);
        }
    }

    return CreateAndUseNewMemoryBlock(queueRecord, std::move(sync), category, alignment, leastSize);
}

D3D12_HEAP_FLAGS TransientResourcePoolDetail::MemoryBlockPool::TranslateHeapFlag(Category category)
{
    switch(category)
    {
    case Category::Buffer:         return D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    case Category::NonRTDSTexture: return D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
    case Category::RTDSTexture:    return D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
    case Category::General:        return D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
    }
    Unreachable();
}

const TransientResourcePoolDetail::MemoryBlockPool::MemoryBlock &
    TransientResourcePoolDetail::MemoryBlockPool::CreateAndUseNewMemoryBlock(
        QueueRecord       &queueRecord,
        RC<QueueSyncQuery> sync,
        Category           category,
        HeapAlignment      alignment,
        size_t             leastSize)
{
    size_t size = memoryBlockSizeHint_;
    if(size < leastSize)
    {
        size = 1;
        while(size < leastSize)
        {
            size += size;
        }
    }

    D3D12MA::ALLOCATION_DESC allocDesc;
    allocDesc.Flags          = D3D12MA::ALLOCATION_FLAG_CAN_ALIAS;
    allocDesc.HeapType       = D3D12_HEAP_TYPE_DEFAULT;
    allocDesc.ExtraHeapFlags = TranslateHeapFlag(category);
    allocDesc.CustomPool     = nullptr;
    allocDesc.pPrivateData   = nullptr;

    D3D12_RESOURCE_ALLOCATION_INFO allocInfo;
    allocInfo.SizeInBytes = size;
    allocInfo.Alignment   = alignment == HeapAlignment::MSAA ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT
                                                             : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

    ComPtr<D3D12MA::Allocation> alloc;
    device_->_internalGetAllocator()->AllocateMemory(&allocDesc, &allocInfo, alloc.GetAddressOf());

    MemoryBlock mb;
    mb.queueSync          = std::move(sync);
    mb.size               = alloc->GetSize();
    mb.allocation         = std::move(alloc);
    mb.alignment          = alignment;
    return *queueRecord.usedMemoryBlocks_[std::to_underlying(category)][std::to_underlying(alignment)].insert(mb);
}

void TransientResourcePoolDetail::MemoryBlockPool::RecycleAvailableMemoryBlocks()
{
    for(auto &queueRecord : std::views::values(queueRecords_))
    {
        // Alias barrier between previous uses of these memory blocks are unnecessary as the subsequent access will
        // be performed in different ExecuteCommandLists scope(s).
        // See https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#resource-aliasing
        for(int i = 0; i < 4; ++i)
        {
            for(int j = 0; j < 2; ++j)
            {
                queueRecord.availableMemoryBlocks_[i][j].merge(queueRecord.usedMemoryBlocks_[i][j]);
                queueRecord.usedMemoryBlocks_[i][j].clear();
            }
        }
    }
}

void TransientResourcePoolDetail::MemoryBlockPool::FreeUnusedMemoryBlocks()
{
    for(auto &queueRecord : std::views::values(queueRecords_))
    {
        for(int i = 0; i < 4; ++i)
        {
            for(int j = 0; j < 2; ++j)
            {
                auto &blocks = queueRecord.availableMemoryBlocks_[i][j];
                for(auto it = blocks.begin(); it != blocks.end();)
                {
                    if(it->queueSync->IsSynchronized())
                    {
                        it = blocks.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
        }
    }
}

RTRC_RHI_D3D12_END
