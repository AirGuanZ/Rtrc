#pragma once

#include <Rtrc/Common.h>

RTRC_BEGIN

class Uncopyable
{
public:

    Uncopyable() = default;
    Uncopyable(const Uncopyable &) = delete;
    Uncopyable(Uncopyable &&) noexcept = delete;
    Uncopyable &operator=(const Uncopyable &) = delete;
    Uncopyable &operator=(Uncopyable &&) noexcept = delete;
};

RTRC_END
