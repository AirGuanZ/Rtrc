#pragma once

#include <algorithm>

#include <Rtrc/Common.h>

RTRC_BEGIN

template<int N>
struct TemplateStringParameter
{
    constexpr TemplateStringParameter(char const (&s)[N])
    {
        std::copy_n(s, N, this->data);
    }

    constexpr auto operator<=>(TemplateStringParameter const &) const = default;

    constexpr std::string_view GetString() const
    {
        return std::string_view(data);
    }

    char data[N];
};

template<int N> TemplateStringParameter(char const(&)[N])->TemplateStringParameter<N>;

RTRC_END
