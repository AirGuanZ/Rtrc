#pragma once

#include <Rtrc/Common.h>

RTRC_BEGIN

namespace TypeListDetail
{

    template<typename T>
    constexpr bool Contains()
    {
        return false;
    }

    template<typename T, typename A, typename...Ts>
    constexpr bool Contains()
    {
        return std::is_same_v<T, A> || ::Rtrc::TypeListDetail::Contains<T, Ts...>();
    }

} // namespace TypeListDetail

template<typename...Ts>
struct TypeList
{
    static constexpr size_t Size = std::tuple_size_v<std::tuple<Ts...>>;

    template<size_t I>
    using At = std::tuple_element_t<I, std::tuple<Ts...>>;

    template<typename T>
    static constexpr bool Contains = TypeListDetail::Contains<T, Ts...>();
};

RTRC_END
