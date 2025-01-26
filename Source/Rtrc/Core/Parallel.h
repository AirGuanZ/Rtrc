#pragma once

#include <execution>
#include <ranges>

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

template<std::integral I, typename F>
void ParallelFor(I begin, I end, const F &f)
{
    auto range = std::views::iota(begin, end);
    std::for_each(std::execution::par_unseq, std::begin(range), std::end(range), [&](I index)
    {
        f(index);
    });
}

template<std::integral I, typename F>
void ParallelForDebug(I begin, I end, const F &f)
{
    auto range = std::views::iota(begin, end);
    std::for_each(std::begin(range), std::end(range), [&](I index)
    {
        f(index);
    });
}

RTRC_END
