#pragma once

#include <fmt/format.h>
#include <half.hpp>

#include <cmath>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <version>

#if defined(__cpp_lib_stacktrace)
#include <stacktrace>
#define RTRC_ENABLE_EXCEPTION_STACKTRACE RTRC_DEBUG
#else
#define RTRC_ENABLE_EXCEPTION_STACKTRACE 0
#endif

#include <Rtrc/Core/Macro/AnonymousName.h>

#define RTRC_BEGIN namespace Rtrc {
#define RTRC_END   }

#define RTRC_RHI_BEGIN RTRC_BEGIN namespace RHI {
#define RTRC_RHI_END } RTRC_END

#define RTRC_GEO_BEGIN RTRC_BEGIN namespace Geo {
#define RTRC_GEO_END } RTRC_END

#if defined(DEBUG) || defined(_DEBUG)
#define RTRC_DEBUG 1
#else
#define RTRC_DEBUG 0
#endif

#define RTRC_RG_DEBUG RTRC_DEBUG
#if RTRC_RG_DEBUG
#define RTRC_IF_RG_DEBUG(X) X
#else
#define RTRC_IF_RG_DEBUG(X) 
#endif

#define RTRC_MAYBE_UNUSED(X) ((void)(X))

#if defined(WIN32) || defined(_WIN32)
#define RTRC_IS_WIN32 1
#else
#define RTRC_IS_WIN32 0
#endif

#ifndef RTRC_REFLECTION_TOOL
#define RTRC_REFLECTION_TOOL 0
#endif

#if defined(__INTELLISENSE__) || defined(__RSCPP_VERSION)
#define RTRC_INTELLISENSE 1
#else
#define RTRC_INTELLISENSE 0
#endif

#if RTRC_INTELLISENSE
#define RTRC_INTELLISENSE_SELECT(FOR_INTELLISENSE, FOR_COMPILATION) FOR_INTELLISENSE
#else
#define RTRC_INTELLISENSE_SELECT(FOR_INTELLISENSE, FOR_COMPILATION) FOR_COMPILATION
#endif

RTRC_BEGIN

template<typename T> class Vector2;
template<typename T> class Vector3;
template<typename T> class Vector4;
class Matrix4x4f;

class Exception : public std::runtime_error
{
#if RTRC_ENABLE_EXCEPTION_STACKTRACE
    std::string stacktrace_;
#endif

public:

    explicit Exception(const char *msg)
        : runtime_error(msg)
    {
#if RTRC_ENABLE_EXCEPTION_STACKTRACE
        stacktrace_ = std::to_string(std::stacktrace::current());
#endif
    }

    explicit Exception(const std::string &msg)
        : runtime_error(msg)
    {
#if RTRC_ENABLE_EXCEPTION_STACKTRACE
        stacktrace_ = std::to_string(std::stacktrace::current());
#endif
    }

#if RTRC_ENABLE_EXCEPTION_STACKTRACE
    const std::string &stacktrace() const { return stacktrace_; }
#else
    const std::string &stacktrace() const { static std::string ret; return ret; }
#endif
};

// =============================================== fp16 ===============================================

using float16 = half_float::half;

inline float16 operator "" _f16(long double f) { return float16(static_cast<float>(f)); }
inline float16 operator "" _f16(unsigned long long f) { return float16(static_cast<float>(f)); }

// =============================================== smart pointer ===============================================

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

template<typename T>
struct IsRCImpl : std::false_type { };

template<typename T>
struct IsRCImpl<RC<T>> : std::true_type { };

template<typename T>
constexpr bool IsRC = IsRCImpl<T>::value;

// =============================================== object id ===============================================

class WithUniqueObjectID
{
public:

    struct UniqueId
    {
        uint64_t data = 0;

        auto operator<=>(const UniqueId &) const = default;

        size_t Hash() const noexcept { return std::hash<uint64_t>{}(data); }
    };

    WithUniqueObjectID()
    {
        static std::atomic<uint64_t> nextID = 1;
        uniqueID_.data = nextID++;
    }

    WithUniqueObjectID(const WithUniqueObjectID &) : WithUniqueObjectID() { }

    WithUniqueObjectID &operator=(const WithUniqueObjectID &) { return *this; }

    const UniqueId &GetUniqueID() const { return uniqueID_; }

private:

    UniqueId uniqueID_;
};

using UniqueId = WithUniqueObjectID::UniqueId;

// =============================================== log ===============================================

enum class LogLevel
{
    Debug = 0,
    Release = 1,
    Default = RTRC_DEBUG ? Debug : Release
};

void SetLogLevel(LogLevel level);

void LogVerboseUnformatted(std::string_view msg);
void LogInfoUnformatted(std::string_view msg);
void LogWarningUnformatted(std::string_view msg);
void LogErrorUnformatted(std::string_view msg);

void SetLogIndentSize(unsigned size);
unsigned GetLogIndentSize();
void SetLogIndentLevel(unsigned level);
unsigned GetLogIndentLevel();
void PushLogIndent();
void PopLogIndent();

std::string_view GetLogIndentString();

template<typename...Args>
void LogVerbose(fmt::format_string<Args...> fmtStr, Args&&...args)
{
    Rtrc::LogVerboseUnformatted(fmt::format(fmtStr, std::forward<Args>(args)...));
}

template<typename...Args>
void LogInfo(fmt::format_string<Args...> fmtStr, Args&&...args)
{
    Rtrc::LogInfoUnformatted(fmt::format(fmtStr, std::forward<Args>(args)...));
}

template<typename...Args>
void LogWarning(fmt::format_string<Args...> fmtStr, Args&&...args)
{
    Rtrc::LogWarningUnformatted(fmt::format(fmtStr, std::forward<Args>(args)...));
}

template<typename...Args>
void LogError(fmt::format_string<Args...> fmtStr, Args&&...args)
{
    Rtrc::LogErrorUnformatted(fmt::format(fmtStr, std::forward<Args>(args)...));
}

template<typename T>
void LogVerbose(const T &msg)
{
    Rtrc::LogVerbose("{}", msg);
}

template<typename T>
void LogInfo(const T &msg)
{
    Rtrc::LogInfo("{}", msg);
}

template<typename T>
void LogWarn(const T &msg)
{
    Rtrc::LogWarning("{}", msg);
}

template<typename T>
void LogError(const T &msg)
{
    Rtrc::LogError("{}", msg);
}

class RtrcLogIndentScope
{
public:

    RtrcLogIndentScope(int dummy = 0) { PushLogIndent(); }
    ~RtrcLogIndentScope() { PopLogIndent(); }
    RtrcLogIndentScope(const RtrcLogIndentScope &) = delete;
    RtrcLogIndentScope &operator=(const RtrcLogIndentScope &) = delete;
};

#define RTRC_LOG_SCOPE() ::Rtrc::RtrcLogIndentScope RTRC_ANONYMOUS_NAME(_rtrcLogIndentScope)

#define RTRC_LOG_VERBOSE_SCOPE(...) \
    ::Rtrc::RtrcLogIndentScope RTRC_ANONYMOUS_NAME(_rtrcLogIndentScope)((::Rtrc::LogVerbose(__VA_ARGS__), 0))
#define RTRC_LOG_INFO_SCOPE(...) \
    ::Rtrc::RtrcLogIndentScope RTRC_ANONYMOUS_NAME(_rtrcLogIndentScope)((::Rtrc::LogInfo(__VA_ARGS__), 0))
#define RTRC_LOG_WARN_SCOPE(...) \
    ::Rtrc::RtrcLogIndentScope RTRC_ANONYMOUS_NAME(_rtrcLogIndentScope)((::Rtrc::LogWarn(__VA_ARGS__), 0))
#define RTRC_LOG_ERROR_SCOPE(...) \
    ::Rtrc::RtrcLogIndentScope RTRC_ANONYMOUS_NAME(_rtrcLogIndentScope)((::Rtrc::LogError(__VA_ARGS__), 0))

// =============================================== misc ===============================================

template<typename R = size_t, typename T, size_t N>
constexpr R GetArraySize(const T(&arr)[N])
{
    return static_cast<R>(N);
}

template<typename C, typename M>
constexpr size_t GetMemberOffset(M C:: *p)
{
    return reinterpret_cast<size_t>(&(static_cast<C *>(nullptr)->*p));
}

template<typename T>
constexpr T UpAlignTo(T v, T align)
{
    return (v + (align - 1)) / align * align;
}

template<typename T>
T *AddToPointer(T *pointer, std::ptrdiff_t offsetInBytes)
{
    static_assert(sizeof(size_t) == sizeof(T *));
    return reinterpret_cast<T *>(reinterpret_cast<size_t>(pointer) + offsetInBytes);
}

template<typename T>
T *AddToPointer(T *pointer, size_t offsetInBytes)
{
    static_assert(sizeof(size_t) == sizeof(T *));
    return reinterpret_cast<T *>(reinterpret_cast<size_t>(pointer) + offsetInBytes);
}

template<typename T> requires std::is_scoped_enum_v<T>
constexpr auto EnumCount = std::to_underlying(T::Count);

template<typename T>
constexpr bool AlwaysFalse = false;

template<typename T>
constexpr size_t GetContainerSize(const T &container)
{
    return container.size();
}

template<typename T, size_t N>
constexpr size_t GetContainerSize(const T(&)[N])
{
    return N;
}

#define RTRC_SET_GET(TYPE, PROP_NAME, MEMBER_NAME)                    \
    void Set##PROP_NAME(const TYPE &value) { (MEMBER_NAME) = value; } \
    const TYPE &Get##PROP_NAME() const { return MEMBER_NAME; }        \
    TYPE &Get##PROP_NAME() { return MEMBER_NAME; } 

// Note that this function doesn't convert the input utf-8 string into any other encoding.
// It just copies the bytes from the input string to the output.
inline std::string u8StringToString(const std::u8string &s)
{
    std::string ret;
    ret.resize(s.size());
    std::memcpy(&ret[0], &s[0], s.size());
    return ret;
}

inline float Atan2Safe(float y, float x)
{
    return y == 0 && x == 0 ? 0.0f : std::atan2(y, x);
}

RTRC_END
