#pragma once

#include <Rtrc/Common.h>

RTRC_BEGIN

class TypeIndex
{
public:

    template<typename T>
    static constexpr TypeIndex Get()
    {
        return TypeIndex(typeid(T).name());
    }

    constexpr TypeIndex(const char *name = nullptr)
        : name_(name)
    {
        
    }

    size_t GetHash() const
    {
        return std::hash<std::string_view>()(std::string_view(name_));
    }

    constexpr operator bool() const
    {
        return name_ != nullptr;
    }

    constexpr auto operator<=>(const TypeIndex &rhs) const
    {
        return std::string_view(name_) <=> std::string_view(rhs.name_);
    }

    constexpr bool operator==(const TypeIndex &rhs) const
    {
        return name_ == rhs.name_ || std::string_view(name_) == std::string(rhs.name_);
    }

private:

    const char *name_;
};

RTRC_END

namespace std
{

    template<>
    struct hash<Rtrc::TypeIndex>
    {
        size_t operator()(const Rtrc::TypeIndex &index) const noexcept
        {
            return index.GetHash();
        }
    };

} // namespac estd
