#pragma once

#include <memory>
#include <stdexcept>

#define RTRC_BEGIN namespace Rtrc {
#define RTRC_END   } // namespace Rtrc

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

RTRC_END
