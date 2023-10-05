#pragma once

#include <fmt/format.h>
#include <half.hpp>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <version>

#include <Core/Macro/MacroForEach.h>

#if defined(__cpp_lib_stacktrace)
#include <stacktrace>
#define RTRC_ENABLE_EXCEPTION_STACKTRACE RTRC_DEBUG
#else
#define RTRC_ENABLE_EXCEPTION_STACKTRACE 0
#endif

#define RTRC_BEGIN namespace Rtrc {
#define RTRC_END   }

#define RTRC_RHI_BEGIN RTRC_BEGIN namespace RHI {
#define RTRC_RHI_END } RTRC_END

#define RTRC_RG_BEGIN RTRC_BEGIN namespace RG {
#define RTRC_RG_END } RTRC_END
#define RTRC_RG_DEBUG RTRC_DEBUG

#define RTRC_RENDERER_BEGIN RTRC_BEGIN namespace Renderer {
#define RTRC_RENDERER_END   } RTRC_END

#if defined(DEBUG) || defined(_DEBUG)
#define RTRC_DEBUG 1
#else
#define RTRC_DEBUG 0
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

RTRC_BEGIN

template<typename T> class Vector2;
template<typename T> class Vector3;
template<typename T> class Vector4;
class Matrix4x4f;

struct _rtrcReflStructBaseSuffix { auto operator<=>(const _rtrcReflStructBaseSuffix &) const = default; };
struct _rtrcReflStructBase_rtrc { auto operator<=>(const _rtrcReflStructBase_rtrc &) const = default; };
struct _rtrcReflStructBase_shader
{
    struct _rtrcReflStructBaseShaderTag {};
    using float2   = Vector2<float>;
    using float3   = Vector3<float>;
    using float4   = Vector4<float>;
    using float4x4 = Matrix4x4f;
    using int2     = Vector2<int>;
    using int3     = Vector3<int>;
    using int4     = Vector4<int>;
    using uint     = unsigned int;
    using uint2    = Vector2<unsigned int>;
    using uint3    = Vector3<unsigned int>;
    using uint4    = Vector4<unsigned int>;
    auto operator<=>(const _rtrcReflStructBase_shader &) const = default;
};
template<typename T>
concept RtrcReflShaderStruct = requires { typename T::_rtrcReflStructBaseShaderTag; };

#define rtrc_derive_from_refl_base_impl(name) ::Rtrc::_rtrcReflStructBase_##name,
#define rtrc_derive_from_refl_base(name) rtrc_derive_from_refl_base_impl(name)
#define rtrc_derive_from_refl_bases(...) \
    RTRC_MACRO_FOREACH_1(rtrc_derive_from_refl_base, __VA_ARGS__) ::Rtrc::_rtrcReflStructBaseSuffix

#if RTRC_REFLECTION_TOOL
#define rtrc_apply_refl(name) __attribute__((annotate(#name)))
#define rtrc_refl_impl(...) RTRC_MACRO_FOREACH_1(rtrc_apply_refl, __VA_ARGS__)
#define rtrc_struct(NAME, ...)                                         \
    struct rtrc_refl_impl(rtrc, shader __VA_OPT__(,) __VA_ARGS__) NAME \
        : rtrc_derive_from_refl_bases(rtrc, shader __VA_OPT__(,) __VA_ARGS__)
#else
#if _MSC_VER
// Use '__declspec(empty_bases)' to fix EBO in msvc.
// See https://devblogs.microsoft.com/cppblog/optimizing-the-layout-of-empty-base-classes-in-vs2015-update-2-3/
#define rtrc_struct(NAME, ...)          \
    struct __declspec(empty_bases) NAME \
        : rtrc_derive_from_refl_bases(rtrc, shader __VA_OPT__(,) __VA_ARGS__)
#else
#define rtrc_refl_struct(NAME, ...) \
    struct NAME : rtrc_derive_from_refl_bases(rtrc, shader __VA_OPT__(,) __VA_ARGS__)
#endif
#endif

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

using float16 = half_float::half;

inline float16 operator "" _f16(long double f) { return float16(static_cast<float>(f)); }
inline float16 operator "" _f16(unsigned long long f) { return float16(static_cast<float>(f)); }

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

template<typename T>
struct IsRCImpl : std::false_type { };

template<typename T>
struct IsRCImpl<RC<T>> : std::true_type { };

template<typename T>
constexpr bool IsRC = IsRCImpl<T>::value;

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
    const TYPE &Get##PROP_NAME() const { return MEMBER_NAME; }

// Note that this function doesn't convert the input utf-8 string into any other encoding.
// It just copies the bytes from the input string to the output.
inline std::string u8StringToString(const std::u8string &s)
{
    std::string ret;
    ret.resize(s.size());
    std::memcpy(&ret[0], &s[0], s.size());
    return ret;
}

RTRC_END
