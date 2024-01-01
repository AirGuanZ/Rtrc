#pragma once

#include <cassert>
#include <map>
#include <set>

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

class RangeSet
{
public:

    enum Policy
    {
        BestFit,
        Largest
    };

    using Index = uint32_t;

    static constexpr Index NIL = (std::numeric_limits<Index>::max)();

    RangeSet() = default;
    explicit RangeSet(Index end);
    RangeSet(Index beg, Index end);

    Index Allocate(Index size, Policy policy = BestFit);
    void Free(Index beg, Index end);

    // f is called with f(oldPos, newPos, count)
    // any element is transferred at most once
    template<typename F>
    void Defragmentation(const F &f);

private:

    std::map<Index, std::set<Index>> sizeToOffsets_;
    std::map<Index, Index> offsetToSize_;
};

inline RangeSet::RangeSet(Index end)
    : RangeSet(0, end)
{
    
}

inline RangeSet::RangeSet(Index beg, Index end)
{
    Free(beg, end);
}

inline RangeSet::Index RangeSet::Allocate(Index size, Policy policy)
{
    decltype(sizeToOffsets_)::iterator it;
    if(policy == BestFit)
    {
        it = sizeToOffsets_.upper_bound(size);
        if(it == sizeToOffsets_.end())
        {
            return NIL;
        }
    }
    else
    {
        assert(policy == Largest);
        if(sizeToOffsets_.empty())
        {
            return NIL;
        }
        it = sizeToOffsets_.end();
        if((--it)->first < size)
        {
            return NIL;
        }
    }

    std::set<Index> &set = it->second;
    assert(!set.empty());
    const Index foundBegin = *set.begin();
    const Index foundSize = it->first;

    set.erase(set.begin());
    if(set.empty())
    {
        sizeToOffsets_.erase(it);
    }

    assert(foundSize >= size);
    if(foundSize == size)
    {
        offsetToSize_.erase(foundBegin);
        return foundBegin;
    }

    const Index restSize = foundSize - size;
    sizeToOffsets_[restSize].insert(foundBegin);
    offsetToSize_[foundBegin] = foundSize - size;
    return foundBegin + restSize;
}

inline void RangeSet::Free(Index beg, Index end)
{
    auto prevIt = offsetToSize_.lower_bound(beg);
    auto succIt = prevIt;
    if(succIt != offsetToSize_.end())
    {
        ++succIt;
    }
    const bool hasPrev = prevIt != offsetToSize_.end();
    const bool hasSucc = succIt != offsetToSize_.end();
    assert(!hasPrev || prevIt->first + prevIt->second <= beg);
    assert(!hasSucc || succIt->first >= end);
    const bool mergePrev = hasPrev && prevIt->first + prevIt->second == beg;
    const bool mergeSucc = hasSucc && end == succIt->first;

    auto RemoveFromSizeToOffsetMap = [&](Index size, Index offset)
    {
        auto it = sizeToOffsets_.find(size);
        assert(it != sizeToOffsets_.end());
        assert(it->second.contains(offset));
        it->second.erase(offset);
        if(it->second.empty())
        {
            sizeToOffsets_.erase(it);
        }
    };

    if(mergePrev)
    {
        const Index prevBeg = prevIt->first;
        const Index prevSize = prevIt->second;
        if(mergeSucc)
        {
            const Index succBeg = succIt->first;
            const Index succSize = succIt->second;

            RemoveFromSizeToOffsetMap(prevSize, prevBeg);
            RemoveFromSizeToOffsetMap(succSize, succBeg);
            offsetToSize_.erase(succBeg);

            const Index newBeg = prevBeg;
            const Index newEnd = succBeg + succSize;
            const Index newSize = newEnd - newBeg;
            offsetToSize_[prevBeg] = newSize;
            sizeToOffsets_[newSize].insert(newBeg);
        }
        else
        {
            RemoveFromSizeToOffsetMap(prevSize, prevBeg);
            
            const Index newBeg = prevBeg;
            const Index newEnd = end;
            const Index newSize = newEnd - newBeg;
            offsetToSize_[newBeg] = newSize;
            sizeToOffsets_[newSize].insert(newBeg);
        }
    }
    else
    {
        if(mergeSucc)
        {
            const Index succBeg = succIt->first;
            const Index succSize = succIt->second;

            RemoveFromSizeToOffsetMap(succSize, succBeg);
            offsetToSize_.erase(succBeg);

            const Index newBeg = beg;
            const Index newEnd = succBeg + succSize;
            const Index newSize = newEnd - newBeg;
            offsetToSize_[newBeg] = newSize;
            sizeToOffsets_[newSize].insert(newBeg);
        }
        else
        {
            const Index newSize = end - beg;
            offsetToSize_[beg] = newSize;
            sizeToOffsets_[newSize].insert(beg);
        }
    }
}

template<typename F>
void RangeSet::Defragmentation(const F &f)
{
    static_assert(AlwaysFalse<F>, "To be implemented");
}

RTRC_END
