#pragma once

#include <cstddef>

#include <Rtrc/Common.h>

RTRC_BEGIN

template<typename T>
class ReferenceCountedPtr;

class ReferenceCounted
{
    template<typename T>
    friend class ReferenceCountedPtr;

    void IncreaseCounter() const
    {
        ++counter_;
    }

    // returns new counter value
    uint32_t DecreaseCounter() const
    {
        return --counter_;
    }

    uint32_t GetCounter() const
    {
        return counter_;
    }

    mutable std::atomic<uint32_t> counter_ = 0;
};

template<typename T>
class ReferenceCountedPtr
{
public:

    ReferenceCountedPtr()
        : ptr_(nullptr)
    {

    }

    ReferenceCountedPtr(std::nullptr_t)
        : ReferenceCountedPtr()
    {

    }

    explicit ReferenceCountedPtr(T *ptr)
        : ptr_(ptr)
    {
        if(ptr_)
        {
            ptr_->ReferenceCounted::IncreaseCounter();
        }
    }

    ReferenceCountedPtr(const ReferenceCountedPtr &other)
        : ptr_(other.ptr_)
    {
        if(ptr_)
        {
            ptr_->ReferenceCounted::IncreaseCounter();
        }
    }

    ReferenceCountedPtr(ReferenceCountedPtr &&other) noexcept
        : ptr_(other.ptr_)
    {
        other.ptr_ = nullptr;
    }

    ~ReferenceCountedPtr()
    {
        if(ptr_ && !ptr_->DecreaseCounter())
        {
            delete ptr_;
        }
    }

    ReferenceCountedPtr &operator=(const ReferenceCountedPtr &other)
    {
        ReferenceCountedPtr t(other);
        this->Swap(t);
        return *this;
    }

    ReferenceCountedPtr &operator=(ReferenceCountedPtr &&other) noexcept
    {
        this->Swap(other);
        return *this;
    }

    void Swap(ReferenceCountedPtr &other) noexcept
    {
        std::swap(ptr_, other.ptr_);
    }

    void Reset(T *ptr = nullptr)
    {
        ReferenceCountedPtr t(ptr);
        this->Swap(t);
    }

    operator bool() const
    {
        return ptr_ != nullptr;
    }

    template<typename U> requires std::is_base_of_v<U, T>
    operator ReferenceCountedPtr<U>() const
    {
        U *ptr = ptr_;
        return ReferenceCountedPtr<U>(ptr);
    }

    operator T *() const
    {
        return ptr_;
    }

    T *operator->() const
    {
        return ptr_;
    }

    T *Get() const
    {
        return ptr_;
    }

    T &operator*() const
    {
        return *ptr_;
    }

    uint32_t GetCounter() const
    {
        return ptr_->ReferenceCounted::GetCounter();
    }

private:

    T *ptr_;
};

template<typename U, typename T> requires std::is_base_of_v<T, U>
ReferenceCountedPtr<U> DynamicCast(const ReferenceCountedPtr<T> &ptr)
{
    auto castedPtr = dynamic_cast<U *>(ptr.Get());
    return ReferenceCountedPtr<U>(castedPtr);
}

template<typename T, typename...Args>
ReferenceCountedPtr<T> MakeReferenceCountedPtr(Args&&...args)
{
    T *object = new T(std::forward<Args>(args)...);
    return ReferenceCountedPtr<T>(object);
}

template<typename A, typename B>
bool operator<(const ReferenceCountedPtr<A> &a, const ReferenceCountedPtr<B> &b)
{
    return a.Get() < b.Get();
}

template<typename A, typename B>
bool operator==(const ReferenceCountedPtr<A> &a, const ReferenceCountedPtr<B> &b)
{
    return a.Get() == b.Get();
}

template<typename A, typename B>
bool operator!=(const ReferenceCountedPtr<A> &a, const ReferenceCountedPtr<B> &b)
{
    return a.Get() != b.Get();
}

RTRC_END
