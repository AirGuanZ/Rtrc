#pragma once

#include <Core/Common.h>
#include <Core/Macro/MacroForEach.h>

RTRC_BEGIN

template<typename T>
concept MemberSwappable = requires(T & a, T & b) { a.swap(b); };

template<typename T>
concept MemberSwappable2 = requires(T & a, T & b) { a.Swap(b); };

template<typename T> requires MemberSwappable<T>
void Swap(T &a, T &b) noexcept(noexcept(a.swap(b)))
{
    a.swap(b);
}

template<typename T> requires !MemberSwappable<T> && MemberSwappable2<T>
void Swap(T &a, T &b) noexcept(noexcept(a.Swap(b)))
{
    a.Swap(b);
}

template<typename T> requires !MemberSwappable<T> && !MemberSwappable2<T>
void Swap(T &a, T &b) noexcept(noexcept(std::swap(a, b)))
{
    std::swap(a, b);
}

template<typename T, typename M>
void SwapMember(T &a, T &b, M T::*m) noexcept(noexcept(Rtrc::Swap(a.*m, b.*m)))
{
    Rtrc::Swap(a.*m, b.*m);
}

#define RTRC_SWAP_MEMBER(A, B, M) (::Rtrc::SwapMember(A, B, &std::remove_cvref_t<decltype(A)>::M));
#define RTRC_SWAP_MEMBERS(A, B, ...) ([&]{ RTRC_MACRO_FOREACH_3(RTRC_SWAP_MEMBER, A, B, __VA_ARGS__) }())

RTRC_END
