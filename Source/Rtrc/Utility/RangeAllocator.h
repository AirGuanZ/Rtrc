#pragma once

#include <cassert>
#include <map>

#include <Rtrc/Common.h>

RTRC_BEGIN

class RangeAllocator
{
public:

    enum class Policy
    {
        BestFit,
        Largest,
    };

    using Index = uint32_t;

    static constexpr Index NIL = (std::numeric_limits<Index>::max)();

    RangeAllocator() = default;
    explicit RangeAllocator(Index end);
    RangeAllocator(Index begin, Index end);

    Index Allocate(Index count, Policy policy = Policy::BestFit);
    void Free(Index beg, Index end);

    // f is called with f(oldPos, newPos, count)
    // any element is transferred at most once
    template<typename F>
    void Defragmentation(const F &f);

private:

    std::multimap<Index, Index> freeBlockSizeToBegin_;
    std::map<Index, Index> freeBlockBeginToSize_;
};

inline RangeAllocator::RangeAllocator(Index end)
    : RangeAllocator(0, end)
{
    
}

inline RangeAllocator::RangeAllocator(Index begin, Index end)
{
    Free(begin, end);
}

inline RangeAllocator::Index RangeAllocator::Allocate(Index count, Policy policy)
{
    std::multimap<Index, Index>::iterator it;
    if(policy == Policy::BestFit)
    {
        it = freeBlockSizeToBegin_.lower_bound(count);
        if(it == freeBlockSizeToBegin_.end())
        {
            return NIL;
        }
    }
    else
    {
        assert(policy == Policy::Largest);
        if(freeBlockSizeToBegin_.empty())
        {
            return NIL;
        }
        it = freeBlockSizeToBegin_.end();
        if((--it)->first < count)
        {
            return NIL;
        }
    }
    const Index foundBegin = it->second;
    const Index foundCount = it->first;
    freeBlockSizeToBegin_.erase(it);
    const Index restCount = foundCount - count;
    if(restCount)
    {
        // Returns [foundBegin + restCount, foundBegin + foundCount)
        freeBlockBeginToSize_[foundBegin] = restCount;
        freeBlockSizeToBegin_.insert({ restCount, foundBegin });
        return foundBegin + restCount;
    }
    freeBlockBeginToSize_.erase(foundBegin);
    return foundBegin;
}

template<typename F>
void RangeAllocator::Defragmentation(const F &f)
{
    static_assert(AlwaysFalse<F>, "To be implemented");
}

RTRC_END
