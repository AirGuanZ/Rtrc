#pragma once

#include <Rtrc/Common.h>

RTRC_BEGIN

template<typename Enum>
class EnumFlags
{
    static_assert(std::is_enum_v<Enum> || std::is_scoped_enum_v<Enum>);

public:

    using Integer = std::underlying_type_t<Enum>;
    
    constexpr EnumFlags(Enum value = static_cast<Enum>(0));

    constexpr EnumFlags(Integer value);

    constexpr bool contains(EnumFlags value) const;

    constexpr operator bool() const;

    constexpr operator Integer() const;

    constexpr Integer GetInteger() const;

    constexpr auto operator<=>(const EnumFlags &) const = default;

    Integer value;
};

template<typename T>
constexpr EnumFlags<T> operator|(const EnumFlags<T> &a, const EnumFlags<T> &b);

template<typename T>
constexpr EnumFlags<T> operator|(const EnumFlags<T> &a, const T &b);

template<typename T>
constexpr EnumFlags<T> operator|(const T &a, const EnumFlags<T> &b);

template<typename T>
constexpr EnumFlags<T> operator&(const EnumFlags<T> &a, const EnumFlags<T> &b);

template<typename T>
constexpr EnumFlags<T> operator&(const EnumFlags<T> &a, const T &b);

template<typename T>
constexpr EnumFlags<T> operator&(const T &a, const EnumFlags<T> &b);

template<typename T>
constexpr bool operator==(const EnumFlags<T> &a, const EnumFlags<T> &b);

template<typename T, typename R>
constexpr EnumFlags<T> &operator|=(EnumFlags<T> &a, R b);

#define RTRC_DEFINE_ENUM_FLAGS(Enum)                                    \
    inline constexpr ::Rtrc::EnumFlags<Enum> operator|(Enum a, Enum b)  \
    {                                                                   \
        return ::Rtrc::EnumFlags<Enum>(a) | ::Rtrc::EnumFlags<Enum>(b); \
    }

template<typename Enum>
constexpr EnumFlags<Enum>::EnumFlags(Enum value)
    : EnumFlags(static_cast<Integer>(value))
{
    
}

template<typename Enum>
constexpr EnumFlags<Enum>::EnumFlags(Integer value)
    : value(value)
{
    
}

template<typename Enum>
constexpr bool EnumFlags<Enum>::contains(EnumFlags val) const
{
    return (this->value & val.GetInteger()) == val.GetInteger();
}

template<typename Enum>
constexpr EnumFlags<Enum>::operator bool() const
{
    return value != 0;
}

template<typename Enum>
constexpr EnumFlags<Enum>::operator std::underlying_type_t<Enum>() const
{
    return value;
}

template<typename Enum>
constexpr typename EnumFlags<Enum>::Integer EnumFlags<Enum>::GetInteger() const
{
    return value;
}

template<typename T>
constexpr EnumFlags<T> operator|(const EnumFlags<T> &a, const EnumFlags<T> &b)
{
    return EnumFlags<T>(a.GetInteger() | b.GetInteger());
}

template<typename T>
constexpr EnumFlags<T> operator|(const EnumFlags<T> &a, const T &b)
{
    return a | EnumFlags<T>(b);
}

template<typename T>
constexpr EnumFlags<T> operator|(const T &a, const EnumFlags<T> &b)
{
    return EnumFlags<T>(a) | b;
}

template<typename T>
constexpr EnumFlags<T> operator&(const EnumFlags<T> &a, const EnumFlags<T> &b)
{
    return EnumFlags<T>(a.GetInteger() & b.GetInteger());
}

template<typename T>
constexpr EnumFlags<T> operator&(const EnumFlags<T> &a, const T &b)
{
    return a & EnumFlags<T>(b);
}

template<typename T>
constexpr EnumFlags<T> operator&(const T &a, const EnumFlags<T> &b)
{
    return EnumFlags<T>(a) & b;
}

template<typename T>
constexpr bool operator==(const EnumFlags<T> &a, const EnumFlags<T> &b)
{
    return a.GetInteger() == b.GetInteger();
}

template<typename T, typename R>
constexpr EnumFlags<T> &operator|=(EnumFlags<T> &a, R b)
{
    return a = a | b;
}

RTRC_END
