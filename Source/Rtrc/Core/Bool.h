#pragma once

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

class Bool
{
    uint8_t value_;

public:

    Bool(): Bool(false) { }
    Bool(bool value) : value_(value) { }

    Bool &operator=(const Bool &) noexcept = default;
    Bool &operator=(bool value) noexcept { value_ = value; return *this; }

    operator bool() const { return value_; }
    operator int() const { return value_; }

    auto operator<=>(const Bool &) const = default;
};

RTRC_END
