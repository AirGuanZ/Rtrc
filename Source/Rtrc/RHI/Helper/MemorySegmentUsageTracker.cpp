#include <Rtrc/RHI/Helper/MemorySegmentUsageTracker.h>

RTRC_RHI_BEGIN

std::vector<int> MemorySegmentUsageTracker::AddNewUser(
    const void *memoryBlockKey, int resource, size_t offset, size_t size)
{
    // Initialize new memory block if necessary

    auto memoryBlockIt = memoryBlocks_.find(memoryBlockKey);
    if(memoryBlockIt == memoryBlocks_.end())
    {
        MemoryBlock newMemoryBlock;
        newMemoryBlock.segments.insert({ 0, offset + size, -1 });
        memoryBlockIt = memoryBlocks_.insert({ memoryBlockKey, std::move(newMemoryBlock) }).first;
    }

    // Extend existing memory block if necessary

    MemoryBlock &memoryBlock = memoryBlockIt->second;
    {
        if(auto &lastSeg = *memoryBlock.segments.rbegin(); lastSeg.end < offset + size)
        {
            if(lastSeg.resource < 0)
            {
                lastSeg.end = offset + size;
            }
            else
            {
                const size_t begin = lastSeg.end;
                const size_t end = offset + size;
                memoryBlock.segments.insert({ begin, end, -1 });
            }
        }
    }

    // Find first segment whose end is greater than offset

    const MemoryBlockSegment dummyKey1 = { .begin = 0, .end = offset, .resource = -1 };
    auto firstIt = std::upper_bound(
        memoryBlock.segments.begin(), memoryBlock.segments.end(), dummyKey1,
        [](const MemoryBlockSegment &lhs, const MemoryBlockSegment &rhs)
        {
            return lhs.end < rhs.end;
        });

    // Find first segment whose end is greater or equal than offset + size

    const MemoryBlockSegment dummyKey2 = { .begin = 0, .end = offset + size, .resource = -1 };
    auto lastIt = std::lower_bound(
        memoryBlock.segments.begin(), memoryBlock.segments.end(), dummyKey2,
        [](const MemoryBlockSegment &lhs, const MemoryBlockSegment &rhs)
        {
            return lhs.end < rhs.end;
        });

    assert(firstIt != memoryBlock.segments.end());
    assert(lastIt != memoryBlock.segments.end());

    // Generate dependencies

    std::vector<int> dependencies;
    for(auto it = firstIt; ; ++it)
    {
        if(it->resource >= 0)
        {
            dependencies.push_back(it->resource);
        }
        if(it == lastIt)
        {
            break;
        }
    }

    // Create new segments

    StaticVector<MemoryBlockSegment, 3> replacementSegments;
    if(firstIt->begin < offset)
    {
        replacementSegments.PushBack(
            {
                .begin = firstIt->begin,
                .end = offset,
                .resource = firstIt->resource
            });
    }
    replacementSegments.PushBack(
        {
            .begin = offset,
            .end = offset + size,
            .resource = resource
        });
    if(lastIt->end > offset + size)
    {
        replacementSegments.PushBack(
            {
                .begin = offset + size,
                .end = lastIt->end,
                .resource = lastIt->resource
            });
    }

    memoryBlock.segments.erase(firstIt, std::next(lastIt));
    memoryBlock.segments.insert_range(replacementSegments);

    return dependencies;
}

RTRC_RHI_END
