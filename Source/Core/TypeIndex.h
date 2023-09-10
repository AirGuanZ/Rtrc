#pragma once

#include <Core/String.h>

RTRC_BEGIN

class TypeIndex
{
public:

    template<typename T>
    static constexpr TypeIndex Get()
    {
        return TypeIndex(typeid(T).name());
    }

    constexpr explicit TypeIndex(const char *name = nullptr)
    {
        if(name)
        {
            name_ = name;
            hash_ = Hash(name_);
        }
        else
        {
            hash_ = 0;
        }
    }

    constexpr size_t GetHash() const
    {
        return hash_;
    }

    constexpr operator bool() const
    {
        return !name_.empty();
    }

    constexpr auto operator<=>(const TypeIndex &rhs) const
    {
        return hash_ == rhs.hash_ ? (name_ <=> rhs.name_) : (hash_ <=> rhs.hash_);
    }

    constexpr bool operator==(const TypeIndex &rhs) const
    {
        return hash_ == rhs.hash_ && name_.data() == rhs.name_.data() && name_ == rhs.name_;
    }

private:

    uint32_t hash_;
    std::string_view name_;
};

RTRC_END

template<>
struct std::hash<Rtrc::TypeIndex>
{
    size_t operator()(const Rtrc::TypeIndex &index) const noexcept
    {
        return index.GetHash();
    }
};
