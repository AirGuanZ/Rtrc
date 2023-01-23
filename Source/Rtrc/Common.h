#pragma once

#include <fmt/format.h>

#include <memory>
#include <stdexcept>

#define RTRC_BEGIN namespace Rtrc {
#define RTRC_END   }

#define RTRC_RHI_BEGIN RTRC_BEGIN namespace RHI {
#define RTRC_RHI_END } RTRC_END

#define RTRC_RG_BEGIN RTRC_BEGIN namespace RG {
#define RTRC_RG_END } RTRC_END

#define RTRC_RDG_BEGIN RTRC_BEGIN namespace RDG {
#define RTRC_RDG_END } RTRC_END

#if defined(DEBUG) || defined(_DEBUG)
#define RTRC_DEBUG 1
#else
#define RTRC_DEBUG 0
#endif

#define RTRC_MAYBE_UNUSED(X) ((void)(X))

RTRC_BEGIN

class Exception : public std::runtime_error
{
public:

    using runtime_error::runtime_error;
};

template<typename T>
using Box = std::unique_ptr<T>;

template<typename T>
using RC = std::shared_ptr<T>;

template<typename D, typename T>
Box<D> DynamicCast(Box<T> ptr)
{
    if(auto ret = dynamic_cast<D*>(ptr.get()))
    {
        ptr.release();
        return Unique<D>(ret);
    }
    return nullptr;
}

template<typename D, typename T>
RC<D> DynamicCast(RC<T> ptr)
{
    return std::dynamic_pointer_cast<D>(std::move(ptr));
}

template<typename T>
RC<T> BoxToRC(Box<T> unique)
{
    return RC<T>(unique.release());
}

template<typename T>
auto ToRC(T &&value)
{
    using TT = std::remove_cvref_t<T>;
    return std::make_shared<TT>(std::forward<T>(value));
}

template<typename T, typename...Args>
RC<T> MakeRC(Args&&...args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T, typename...Args>
Box<T> MakeBox(Args&&...args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename R = size_t, typename T, size_t N>
constexpr R GetArraySize(const T (&arr)[N])
{
    return static_cast<R>(N);
}

template<typename C, typename M>
constexpr size_t GetMemberOffset(M C::*p)
{
    return reinterpret_cast<size_t>(&(static_cast<C*>(nullptr)->*p));
}

template<typename T>
constexpr T UpAlignTo(T v, T align)
{
    return (v + (align - 1)) / align * align;
}

template<typename T>
constexpr bool AlwaysFalse = false;

template<typename T> requires std::is_scoped_enum_v<T>
constexpr auto EnumCount = std::underlying_type_t<T>(T::Count);

template<typename T> requires std::is_enum_v<T>
constexpr auto EnumToInt(T e)
{
    return static_cast<std::underlying_type_t<T>>(e);
}

class WithUniqueObjectID
{
public:

    struct UniqueID
    {
        uint64_t data = 0;

        auto operator<=>(const UniqueID &) const = default;

        size_t Hash() const noexcept { return std::hash<uint64_t>{}(data); }
    };

    WithUniqueObjectID()
    {
        static std::atomic<uint64_t> nextID = 1;
        uniqueID_.data = nextID++;
    }

    WithUniqueObjectID(const WithUniqueObjectID &) : WithUniqueObjectID() { }

    WithUniqueObjectID &operator=(const WithUniqueObjectID &) { return *this; }

    const UniqueID &GetUniqueID() const { return uniqueID_; }

private:

    UniqueID uniqueID_;
};

enum class LogLevel
{
    Debug = 0,
    Release,
    Default = RTRC_DEBUG ? Debug : Release
};

void SetLogLevel(LogLevel level);

void LogDebugUnformatted(std::string_view msg);
void LogInfoUnformatted(std::string_view msg);
void LogWarningUnformatted(std::string_view msg);
void LogErrorUnformatted(std::string_view msg);

template<typename...Args>
void LogDebug(fmt::format_string<Args...> fmtStr, Args&&...args)
{
    Rtrc::LogDebugUnformatted(fmt::format(fmtStr, std::forward<Args>(args)...));
}

template<typename...Args>
void LogInfo(fmt::format_string<Args...> fmtStr, Args&&...args)
{
    Rtrc::LogInfoUnformatted(fmt::format(fmtStr, std::forward<Args>(args)...));
}

template<typename...Args>
void LogWarn(fmt::format_string<Args...> fmtStr, Args&&...args)
{
    Rtrc::LogWarningUnformatted(fmt::format(fmtStr, std::forward<Args>(args)...));
}

template<typename...Args>
void LogError(fmt::format_string<Args...> fmtStr, Args&&...args)
{
    Rtrc::LogErrorUnformatted(fmt::format(fmtStr, std::forward<Args>(args)...));
}

template<typename T>
void LogDebug(const T &msg)
{
    Rtrc::LogDebug("{}", msg);
}

template<typename T>
void LogInfo(const T &msg)
{
    Rtrc::LogInfo("{}", msg);
}

template<typename T>
void LogWarn(const T &msg)
{
    Rtrc::LogWarn("{}", msg);
}

template<typename T>
void LogError(const T &msg)
{
    Rtrc::LogError("{}", msg);
}

RTRC_END
