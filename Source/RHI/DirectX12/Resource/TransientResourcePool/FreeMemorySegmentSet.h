#pragma once

#include <RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

namespace TransientResourcePoolDetail
{

    class FreeMemorySegmentSet : public Uncopyable
    {
    public:

        struct MemorySegment
        {
            D3D12MA::Allocation *allocation;
            size_t offset;
            size_t size;
        };

        std::optional<MemorySegment> Allocate(size_t size, size_t alignment);

        void Free(const MemorySegment &memorySegment);

    private:

        struct Segment;

        struct Offset
        {
            D3D12MA::Allocation *allocation;
            size_t offsetInAllocation;
            auto operator<=>(const Offset &) const = default;
        };

        using OffsetToSegment = std::map<Offset, Segment>;
        using SizeToOffset = std::multimap<size_t, OffsetToSegment::iterator>;

        struct Segment
        {
            size_t size;
            SizeToOffset::iterator sizeToIffsetIterator;
        };

        void RemoveSegment(OffsetToSegment::iterator it);

        OffsetToSegment offsetToSegment_;
        SizeToOffset sizeToOffset_;
    };

} // namespace namespace TransientResourcePoolDetail

RTRC_RHI_D3D12_END
