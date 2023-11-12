#pragma once

#include <RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

namespace TransientResourcePoolDetail
{

    class MemorySegmentUsageTracker
    {
    public:

        // Returns all dependent resources
        std::vector<int> AddNewUser(const void *memoryBlock, int resource, size_t offset, size_t size);

    private:

        struct MemoryBlockSegment
        {
            size_t begin;
            mutable size_t end;
            mutable int resource;

            auto operator<=>(const MemoryBlockSegment &rhs) const { return begin <=> rhs.begin; }
        };

        struct MemoryBlock
        {
            std::set<MemoryBlockSegment> segments;
        };

        std::map<const void *, MemoryBlock> memoryBlocks_;
    };

} // namespace TransientResourcePoolDetail

RTRC_RHI_D3D12_END
