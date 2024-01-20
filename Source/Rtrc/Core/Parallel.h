#pragma once

#include <tbb/parallel_for.h>

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

template<std::integral I, typename F>
void ParallelFor(I begin, I end, const F &f)
{
    tbb::parallel_for(tbb::blocked_range<I>(begin, end), [&](tbb::blocked_range<I> range)
    {
        for(I i : range)
        {
            f(i);
        }
    });
}

RTRC_END
