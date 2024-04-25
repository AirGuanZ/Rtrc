#pragma once

#include <type_traits>

#include <Rtrc/Core/Serialization/Serialize.h>

RTRC_BEGIN

#define RTRC_DEFINE_ENUM_FLAGS(Enum)                                                                                               \
    class EnumFlags##Enum                                                                                                          \
    {                                                                                                                              \
        static_assert(std::is_enum_v<Enum> || std::is_scoped_enum_v<Enum>);                                                        \
                                                                                                                                   \
    public:                                                                                                                        \
        using enum Enum;                                                                                                           \
        using Bits = Enum;                                                                                                         \
        using Integer = std::underlying_type_t<Enum>;                                                                              \
        constexpr EnumFlags##Enum(Enum value = static_cast<Enum>(0)) : value(value) { }                                            \
        constexpr EnumFlags##Enum(Integer value) : value(static_cast<Enum>(value)) { }                                             \
        constexpr bool Contains(EnumFlags##Enum val) const { return (this->GetInteger() & val.GetInteger()) == val.GetInteger(); } \
        constexpr operator bool() const { return GetInteger() != 0; }                                                              \
        constexpr operator Integer() const { return static_cast<Integer>(value); }                                                 \
        constexpr Integer GetInteger() const { return static_cast<Integer>(value); }                                               \
        size_t Hash() const { return std::hash<Integer>{}(GetInteger()); }                                                         \
        constexpr auto operator<=>(const EnumFlags##Enum &) const = default;                                                       \
        Enum value;                                                                                                                \
        RTRC_AUTO_SERIALIZE(value);                                                                                                \
    };                                                                                                                             \
    constexpr EnumFlags##Enum operator|(const EnumFlags##Enum &a, const EnumFlags##Enum &b)                                        \
    {                                                                                                                              \
        return EnumFlags##Enum(a.GetInteger() | b.GetInteger());                                                                   \
    }                                                                                                                              \
    inline constexpr EnumFlags##Enum operator|(Enum a, Enum b)                                                                     \
    {                                                                                                                              \
        return EnumFlags##Enum(a) | EnumFlags##Enum(b);                                                                            \
    }                                                                                                                              \
    constexpr EnumFlags##Enum operator|(const EnumFlags##Enum &a, const Enum &b)                                                   \
    {                                                                                                                              \
        return a | EnumFlags##Enum(b);                                                                                             \
    }                                                                                                                              \
    constexpr EnumFlags##Enum operator|(const Enum &a, const EnumFlags##Enum &b)                                                   \
    {                                                                                                                              \
        return EnumFlags##Enum(a) | b;                                                                                             \
    }                                                                                                                              \
    constexpr EnumFlags##Enum operator&(const EnumFlags##Enum &a, const EnumFlags##Enum &b)                                        \
    {                                                                                                                              \
        return EnumFlags##Enum(a.GetInteger() & b.GetInteger());                                                                   \
    }                                                                                                                              \
    constexpr EnumFlags##Enum operator&(const EnumFlags##Enum &a, const Enum &b)                                                   \
    {                                                                                                                              \
        return a & EnumFlags##Enum(b);                                                                                             \
    }                                                                                                                              \
    constexpr EnumFlags##Enum operator&(const Enum &a, const EnumFlags##Enum &b)                                                   \
    {                                                                                                                              \
        return EnumFlags##Enum(a) & b;                                                                                             \
    }                                                                                                                              \
    constexpr EnumFlags##Enum &operator|=(EnumFlags##Enum &a, Enum b)                                                              \
    {                                                                                                                              \
        return a = a | b;                                                                                                          \
    }                                                                                                                              \
    constexpr EnumFlags##Enum &operator|=(EnumFlags##Enum &a, EnumFlags##Enum b)                                                   \
    {                                                                                                                              \
        return a = a | b;                                                                                                          \
    }                                                                                                                              \
    constexpr EnumFlags##Enum operator~(EnumFlags##Enum v)                                                                         \
    {                                                                                                                              \
        return EnumFlags##Enum(~v.GetInteger());                                                                                   \
    }

RTRC_END
