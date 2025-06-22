#pragma once

#include <concepts>
#include <vector>

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

#define RTRC_NAIVE_TRANSFER(...) \
    RTRC_CUSTOM_TRANSFER() { RTRC_TRANSFER(__VA_ARGS__); } void _rtrcSimpleTransferNeedSemiComma()

#define RTRC_CUSTOM_TRANSFER() \
    template<typename Archive> \
    void Transfer(Archive &ar)

#define RTRC_TRANSFER_SINGLE(X) RTRC_TRANSFER_SINGLE_IMPL(X)
#define RTRC_TRANSFER_SINGLE_IMPL(X) ar.Transfer(#X, X);
#define RTRC_TRANSFER(...) ([&]{RTRC_MACRO_FOREACH_1(RTRC_TRANSFER_SINGLE, __VA_ARGS__)}())
#define RTRC_TRANSFER_TUPLE(NAME, ...) (ar.TransferTuple(NAME, __VA_ARGS__))

template<typename T>
struct ArchiveTransferTrait;

template<typename Archive, bool IsArchiveForReading>
class ArchiveCommon
{
public:

    template<typename T>
    void Transfer(std::string_view name, T &object)
    {
        auto ar = static_cast<Archive *>(this);
        ArchiveTransferTrait<T>::Transfer(*ar, name, object);
    }

    template<typename T>
    void Transfer(std::string_view name, const T &object)
    {
        auto ar = static_cast<Archive *>(this);
        ArchiveTransferTrait<T>::Transfer(*ar, name, object);
    }

    template<typename...Ts>
    void TransferTuple(std::string_view name, Ts&...elements)
    {
        auto ar = static_cast<Archive *>(this);
        if(ar->BeginTransferTuple(name))
        {
            int i = 0;
            (ar->Transfer(std::to_string(i++), elements), ...);
            (void)i;
            ar->EndTransferTuple();
        }
    }

    template<typename...Ts>
    void TransferTuple(std::string_view name, const Ts&...elements)
    {
        auto ar = static_cast<Archive *>(this);
        if(ar->BeginTransferTuple(name))
        {
            int i = 0;
            (ar->Transfer(std::to_string(i++), elements), ...);
            (void)i;
            ar->EndTransferTuple();
        }
    }

    bool BeginTransferTuple(std::string_view name, int size)
    {
        auto ar = static_cast<Archive *>(this);
        return ar->BeginTransferObject(name);
    }

    void EndTransferTuple()
    {
        auto ar = static_cast<Archive *>(this);
        ar->EndTransferObject();
    }

    static constexpr bool StaticIsReading() { return IsArchiveForReading; }
    static constexpr bool StaticIsWriting() { return !IsArchiveForReading; }
    constexpr bool IsReading() const { return IsArchiveForReading; }
    constexpr bool IsWriting() const { return !IsArchiveForReading; }
};

template<typename T>
struct ArchiveTransferTrait
{
    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, T& object)
    {
        if(ar.BeginTransferObject(name))
        {
            object.Transfer(ar);
            ar.EndTransferObject();
        }
    }

    template<typename Archive> requires requires{ std::declval<const T &>().Transfer(std::declval<Archive &>()); }
    static void Transfer(Archive &ar, std::string_view name, const T &object)
    {
        static_assert(Archive::StaticIsWriting());
        if(ar.BeginTransferObject(name))
        {
            object.Transfer(ar);
            ar.EndTransferObject();
        }
    }
};

template<typename T, size_t N>
struct ArchiveTransferTrait<T[N]>
{
    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, T (&array)[N])
    {
        for(size_t i = 0; i < N; ++i)
        {
            ar.Transfer(std::format("{}[{}]", name, i), array[i]);
        }
    }

    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, const T(&array)[N])
    {
        static_assert(Archive::StaticIsWriting());
        for(size_t i = 0; i < N; ++i)
        {
            ar.Transfer(std::format("{}[{}]", name, i), array[i]);
        }
    }
};

template<std::integral T>
struct ArchiveTransferTrait<T>
{
    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, T &object)
    {
        ar.TransferBuiltin(name, object);
    }

    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, const T &object)
    {
        static_assert(Archive::StaticIsWriting());
        ar.TransferBuiltin(name, object);
    }
};

template<std::floating_point T>
struct ArchiveTransferTrait<T>
{
    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, T &object)
    {
        ar.TransferBuiltin(name, object);
    }

    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, const T &object)
    {
        static_assert(Archive::StaticIsWriting());
        ar.TransferBuiltin(name, object);
    }
};

template<typename T> requires std::is_enum_v<T>
struct ArchiveTransferTrait<T>
{
    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, T &object)
    {
        if(ar.IsReading())
        {
            std::underlying_type_t<T> value = {};
            ar.TransferBuiltin(name, value);
            object = static_cast<T>(value);
        }
        else
        {
            auto value = std::to_underlying(object);
            ar.TransferBuiltin(name, value);
        }
    }

    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, const T &object)
    {
        static_assert(Archive::StaticIsWriting());
        auto value = std::to_underlying(object);
        ar.TransferBuiltin(name, value);
    }
};

template<>
struct ArchiveTransferTrait<std::string>
{
    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, std::string &object)
    {
        ar.TransferBuiltin(name, object);
    }

    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, const std::string &object)
    {
        static_assert(Archive::StaticIsWriting());
        ar.TransferBuiltin(name, object);
    }
};

template<typename T>
struct ArchiveTransferTrait<std::vector<T>>
{
    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, std::vector<T> &object)
    {
        if(ar.BeginTransferObject(name))
        {
            uint64_t size = object.size();
            ar.Transfer("size", size);
            if(ar.DidReadLastProperty())
            {
                object.resize(size);
            }
            for(size_t i = 0; i < object.size(); ++i)
            {
                ar.Transfer(std::to_string(i), object[i]);
            }
            ar.EndTransferObject();
        }
    }

    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, const std::vector<T> &object)
    {
        static_assert(Archive::StaticIsWriting());
        if(ar.BeginTransferObject(name))
        {
            uint64_t size = object.size();
            ar.Transfer("size", size);
            if(ar.DidReadLastProperty())
            {
                object.resize(size);
            }
            for(size_t i = 0; i < object.size(); ++i)
            {
                ar.Transfer(std::to_string(i), object[i]);
            }
            ar.EndTransferObject();
        }
    }
};

RTRC_END
