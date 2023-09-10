#pragma once

#include <algorithm>

#include <Core/Common.h>

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

    char data[N];
};

template<int N> TemplateStringParameter(const char(&)[N]) -> TemplateStringParameter<N>;

RTRC_END
