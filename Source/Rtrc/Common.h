#pragma once

#include <memory>
#include <stdexcept>

#define RTRC_BEGIN namespace Rtrc {
#define RTRC_END   }

#define RTRC_RHI_BEGIN RTRC_BEGIN namespace RHI {
#define RTRC_RHI_END   } RTRC_END

RTRC_BEGIN

class Exception : public std::runtime_error
{
public:

    using runtime_error::runtime_error;
};

template<typename T>
using Unique = std::unique_ptr<T>;

template<typename T>
using Shared = std::shared_ptr<T>;

template<typename T>
using RC = Shared<T>;

template<typename D, typename T>
Unique<D> DynamicCast(Unique<T> ptr)
{
    if(auto ret = dynamic_cast<D*>(ptr.get()))
    {
        ptr.release();
        return Unique<D>(ret);
    }
    return nullptr;
}

template<typename D, typename T>
Shared<D> DynamicCast(Shared<T> ptr)
{
    return std::dynamic_pointer_cast<D>(std::move(ptr));
}

template<typename T, typename...Args>
Unique<T> MakeUnique(Args&&...args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename...Args>
Shared<T> MakeShared(Args&&...args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T>
Shared<T> ToShared(Unique<T> unique)
{
    return Shared<T>(unique.release());
}

template<typename T, typename...Args>
RC<T> MakeRC(Args&&...args)
{
    return MakeShared<T>(std::forward<Args>(args)...);
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

RTRC_END
