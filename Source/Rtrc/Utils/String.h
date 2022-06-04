#pragma once

#include <string>

#include <Rtrc/Common.h>

RTRC_BEGIN

constexpr bool StartsWith(std::string_view str, std::string_view prefix)
{
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

RTRC_END
