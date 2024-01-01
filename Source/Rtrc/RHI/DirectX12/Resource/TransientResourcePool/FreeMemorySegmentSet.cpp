#include <Rtrc/RHI/DirectX12/Resource/TransientResourcePool/FreeMemorySegmentSet.h>

RTRC_RHI_D3D12_BEGIN

std::optional<TransientResourcePoolDetail::FreeMemorySegmentSet::MemorySegment>
    TransientResourcePoolDetail::FreeMemorySegmentSet::Allocate(size_t size, size_t alignment)
{
    auto sizeIt = sizeToOffset_.lower_bound(size);
    while(sizeIt != sizeToOffset_.end())
    {
        auto segmentIt = sizeIt->second;
        const Segment segment = segmentIt->second;
        D3D12MA::Allocation *allocation = segmentIt->first.allocation;
        const size_t unalignedOffset = segmentIt->first.offsetInAllocation;
        const size_t alignedOffset = UpAlignTo(unalignedOffset, alignment);
        const size_t requiredEnd = alignedOffset + size;
        const size_t segmentEnd = unalignedOffset + segment.size;
        if(requiredEnd <= segmentEnd)
        {
            RemoveSegment(segmentIt);
            if(alignedOffset > unalignedOffset)
            {
                Free(MemorySegment
                    {
                        .allocation = allocation,
                        .offset = unalignedOffset,
                        .size = alignedOffset - unalignedOffset
                    });
            }
            if(requiredEnd < segmentEnd)
            {
                Free(MemorySegment
                    {
                        .allocation = allocation,
                        .offset = requiredEnd,
                        .size = segmentEnd - requiredEnd
                    });
            }
            return MemorySegment
                {
                    .allocation = allocation,
                    .offset = alignedOffset,
                    .size = size
                };
        }
        ++sizeIt;
    }
    return std::nullopt;
}

void TransientResourcePoolDetail::FreeMemorySegmentSet::Free(const MemorySegment &memorySegment)
{
    size_t beg = memorySegment.offset, end = memorySegment.offset + memorySegment.size;

    auto succBlockIt = offsetToSegment_.upper_bound({ memorySegment.allocation, beg });
    if(succBlockIt != offsetToSegment_.end() && succBlockIt->first.allocation != memorySegment.allocation)
    {
        succBlockIt = offsetToSegment_.end();
    }

    auto prevBlockIt = offsetToSegment_.lower_bound({ memorySegment.allocation, beg });
    if(prevBlockIt == offsetToSegment_.begin())
    {
        prevBlockIt = offsetToSegment_.end();
    }
    else
    {
        prevBlockIt = std::prev(prevBlockIt);
        if(prevBlockIt != offsetToSegment_.end() && prevBlockIt->first.allocation != memorySegment.allocation)
        {
            prevBlockIt = offsetToSegment_.end();
        }
    }

    if(succBlockIt != offsetToSegment_.end() && succBlockIt->first.offsetInAllocation == end)
    {
        end += succBlockIt->second.size;
        RemoveSegment(succBlockIt);
    }

    if(prevBlockIt != offsetToSegment_.end() && prevBlockIt->first.offsetInAllocation + prevBlockIt->second.size == beg)
    {
        beg = prevBlockIt->first.offsetInAllocation;
        RemoveSegment(prevBlockIt);
    }

    const size_t size = end - beg;
    auto [blockIt, blockInserted] = offsetToSegment_.insert({ { memorySegment.allocation, beg }, Segment{} });
    assert(blockInserted);

    auto sizeIt = sizeToOffset_.insert({ size, blockIt });
    blockIt->second.size                 = size;
    blockIt->second.sizeToIffsetIterator = sizeIt;
}

void TransientResourcePoolDetail::FreeMemorySegmentSet::RemoveSegment(OffsetToSegment::iterator it)
{
    auto sizeIt = it->second.sizeToIffsetIterator;
    offsetToSegment_.erase(it);
    sizeToOffset_.erase(sizeIt);
}

RTRC_RHI_D3D12_END
