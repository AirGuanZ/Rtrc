#pragma once

#include <type_traits>

#include <Core/Common.h>

RTRC_BEGIN

#define RTRC_DEFINE_ENUM_FLAGS(Enum)                                                                                        \
    class EnumFlags##Enum                                                                                                   \
    {                                                                                                                       \
        static_assert(std::is_enum_v<Enum> || std::is_scoped_enum_v<Enum>);                                                 \
                                                                                                                            \
    public:                                                                                                                 \
        using enum Enum;                                                                                                    \
        using Bits = Enum;                                                                                                  \
        using Integer = std::underlying_type_t<Enum>;                                                                       \
        constexpr EnumFlags##Enum(Enum value = static_cast<Enum>(0)) : value(std::to_underlying(value)) { }                 \
        constexpr EnumFlags##Enum(Integer value) : value(value) { }                                                         \
        constexpr bool Contains(EnumFlags##Enum val) const { return (this->value & val.GetInteger()) == val.GetInteger(); } \
        constexpr operator bool() const { return value != 0; }                                                              \
        constexpr operator Integer() const { return value; }                                                                \
        constexpr Integer GetInteger() const { return value; }                                                              \
        size_t Hash() const { return std::hash<Integer>{}(value); }                                                         \
        constexpr auto operator<=>(const EnumFlags##Enum &) const = default;                                                \
        Integer value;                                                                                                      \
    };                                                                                                                      \
    constexpr EnumFlags##Enum operator|(const EnumFlags##Enum &a, const EnumFlags##Enum &b)                                 \
    {                                                                                                                       \
        return EnumFlags##Enum(a.GetInteger() | b.GetInteger());                                                            \
    }                                                                                                                       \
    inline constexpr EnumFlags##Enum operator|(Enum a, Enum b)                                                              \
    {                                                                                                                       \
        return EnumFlags##Enum(a) | EnumFlags##Enum(b);                                                                     \
    }                                                                                                                       \
    constexpr EnumFlags##Enum operator|(const EnumFlags##Enum &a, const Enum &b)                                            \
    {                                                                                                                       \
        return a | EnumFlags##Enum(b);                                                                                      \
    }                                                                                                                       \
    constexpr EnumFlags##Enum operator|(const Enum &a, const EnumFlags##Enum &b)                                            \
    {                                                                                                                       \
        return EnumFlags##Enum(a) | b;                                                                                      \
    }                                                                                                                       \
    constexpr EnumFlags##Enum operator&(const EnumFlags##Enum &a, const EnumFlags##Enum &b)                                 \
    {                                                                                                                       \
        return EnumFlags##Enum(a.GetInteger() & b.GetInteger());                                                            \
    }                                                                                                                       \
    constexpr EnumFlags##Enum operator&(const EnumFlags##Enum &a, const Enum &b)                                            \
    {                                                                                                                       \
        return a & EnumFlags##Enum(b);                                                                                      \
    }                                                                                                                       \
    constexpr EnumFlags##Enum operator&(const Enum &a, const EnumFlags##Enum &b)                                            \
    {                                                                                                                       \
        return EnumFlags##Enum(a) & b;                                                                                      \
    }                                                                                                                       \
    constexpr EnumFlags##Enum &operator|=(EnumFlags##Enum &a, Enum b)                                                       \
    {                                                                                                                       \
        return a = a | b;                                                                                                   \
    }                                                                                                                       \
    constexpr EnumFlags##Enum &operator|=(EnumFlags##Enum &a, EnumFlags##Enum b)                                            \
    {                                                                                                                       \
        return a = a | b;                                                                                                   \
    }

RTRC_END
