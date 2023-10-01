#pragma once

#include <RHI/RHI.h>

RTRC_SHADER_COMPILER_BEGIN

// Note that this function doesn't convert the input utf-8 string into any other encoding.
// It just copies the bytes from the input string to the output.
inline std::string u8StringToString(const std::u8string &s)
{
    std::string ret;
    ret.resize(s.size());
    std::memcpy(&ret[0], &s[0], s.size());
    return ret;
}

RTRC_SHADER_COMPILER_END
