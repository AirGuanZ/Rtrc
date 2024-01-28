#pragma once

#include <Rtrc/RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

namespace TransientResourcePoolDetail
{

    class MemoryBlockPool : public Uncopyable
    {
    public:

        enum class Category
        {
            Buffer,
            NonRTDSTexture,
            RTDSTexture,
            General,
        };

        enum class HeapAlignment
        {
            Regular,
            MSAA
        };

        struct MemoryBlock
        {
            ComPtr<D3D12MA::Allocation> allocation;
            HeapAlignment               alignment;
            size_t                      size = 0;
            RC<QueueSyncQuery>          queueSync;
        };

        MemoryBlockPool(DirectX12Device *device, size_t memoryBlockSizeHint);

        const MemoryBlock &GetMemoryBlock(
            QueueOPtr          queue,
            RC<QueueSyncQuery> sync,
            Category           category,
            HeapAlignment      alignment,
            size_t             leastSize);

        void RecycleAvailableMemoryBlocks();

        void FreeUnusedMemoryBlocks();

    private:

        struct MemoryBlockComp
        {
            bool operator()(const MemoryBlock &lhs, const MemoryBlock &rhs) const { return lhs.size < rhs.size; }
            bool operator()(const MemoryBlock &lhs, size_t             rhs) const { return lhs.size < rhs;      }
            bool operator()(size_t lhs,             const MemoryBlock &rhs) const { return lhs      < rhs.size; }
        };

        struct QueueRecord
        {
            std::multiset<MemoryBlock, MemoryBlockComp> availableMemoryBlocks_[4/*category*/][2/*alignment*/];
            std::multiset<MemoryBlock, MemoryBlockComp> usedMemoryBlocks_[4][2];
        };

        static D3D12_HEAP_FLAGS TranslateHeapFlag(Category category);

        const MemoryBlock &CreateAndUseNewMemoryBlock(
            QueueRecord       &queueRecord,
            RC<QueueSyncQuery> sync,
            Category           category,
            HeapAlignment      alignment,
            size_t             leastSize);

        DirectX12Device *device_;
        size_t memoryBlockSizeHint_;

        std::map<QueueOPtr, QueueRecord> queueRecords_;
    };

} // namespace TransientResourcePoolDetail

RTRC_RHI_D3D12_END
