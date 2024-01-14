#pragma once

#include <algorithm>

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

template<int N>
struct TemplateStringParameter
{
    constexpr TemplateStringParameter(const char (&s)[N])
    {
        std::copy_n(s, N, this->data);
    }

    constexpr auto operator<=>(const TemplateStringParameter &) const = default;

    constexpr std::string_view GetString() const
    {
        return std::string_view(data);
    }

    constexpr operator std::string() const
    {
        return std::string(data);
    }

    constexpr operator std::string_view() const
    {
        return std::string_view(data);
    }

    constexpr operator const char*() const
    {
        return data;
    }

    char data[N];
};

RTRC_END
