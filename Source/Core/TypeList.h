#pragma once

#include <Core/Common.h>

RTRC_BEGIN

template<typename...Ts>
struct TypeList;

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
        return std::is_same_v<T, A> || TypeListDetail::Contains<T, Ts...>();
    }

} // namespace TypeListDetail

template<typename...Ts>
struct TypeList
{
private:

    template<typename...T1s, typename...T2s>
    static TypeList<T1s..., T2s...> CatHelper(TypeList<T1s...> t1, TypeList<T2s...>) { return {}; }

public:

    static constexpr size_t Size = std::tuple_size_v<std::tuple<Ts...>>;

    template<size_t I>
    using At = std::tuple_element_t<I, std::tuple<Ts...>>;

    template<typename T>
    static constexpr bool Contains = TypeListDetail::Contains<T, Ts...>();

    template<typename L2>
    using AppendList = decltype(TypeList::CatHelper(TypeList{}, L2{}));

    template<typename Func>
    using Map = TypeList<typename Func::template Type<Ts>...>;
};

namespace CatTypeListDetail
{

    template<typename L1, typename L2>
    auto CatTypeListAux(L1, L2)
    {
        using RetType = typename TypeList<L1>::template AppendList<L2>;
        return RetType{};
    }

    template<typename L1, typename L2, typename L3, typename...Ls>
    auto CatTypeListAux(L1, L2, L3, Ls...)
    {
        return CatTypeListDetail::CatTypeListAux(CatTypeListDetail::CatTypeListAux(L1{}, L2{}), L3{}, Ls{}...);
    }

} // namespace CatTypeListDetail

template<typename...Ls>
using CatTypeLists = decltype(CatTypeListDetail::CatTypeListAux(Ls{}...));

RTRC_END
