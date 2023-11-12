#pragma once

#include <RHI/DirectX12/Common.h>

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

        enum class Alignment
        {
            Regular,
            MSAA
        };

        struct MemoryBlock
        {
            ComPtr<D3D12MA::Allocation> allocation;
            Alignment alignment;
            int lastActiveSession = -1;
            size_t size = 0;
        };

        MemoryBlockPool(DirectX12Device *device, size_t memoryBlockSizeHint);

        int StartHostSynchronizationSession();

        void CompleteHostSynchronizationSession(int session);

        const MemoryBlock &GetMemoryBlock(Category category, Alignment alignment, size_t leastSize);

    private:

        struct MemoryBlockComp
        {
            bool operator()(const MemoryBlock &lhs, const MemoryBlock &rhs) const
            {
                return lhs.size < rhs.size;
            }

            bool operator()(const MemoryBlock &lhs, size_t rhs) const
            {
                return lhs.size < rhs;
            }

            bool operator()(size_t lhs, const MemoryBlock &rhs) const
            {
                return lhs < rhs.size;
            }
        };

        static D3D12_HEAP_FLAGS TranslateHeapFlag(Category category);

        const MemoryBlock &CreateAndUseNewMemoryBlock(Category category, Alignment alignment, size_t leastSize);

        void RecycleUnusedMemoryBlocks();

        DirectX12Device *device_;
        size_t memoryBlockSizeHint_;

        int synchronizedHostSession_;
        int currentHostSession_;

        std::multiset<MemoryBlock, MemoryBlockComp> availableMemoryBlocks_[4/*category*/][2/*alignment*/];
        std::multiset<MemoryBlock, MemoryBlockComp> usedMemoryBlocks_[4][2];
    };

} // namespace TransientResourcePoolDetail

RTRC_RHI_D3D12_END
