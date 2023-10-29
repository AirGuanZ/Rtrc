#pragma once

#include <Core/SmartPointer/ReferenceCounted.h>

RTRC_BEGIN

template<typename T>
class UPtr
{
public:

    using ElementType = T;

    UPtr() : UPtr(nullptr) { }
    UPtr(std::nullptr_t) : ptr_(nullptr) { }
    UPtr(T *ptr) : ptr_(ptr) { }

    UPtr(const UPtr &) = delete;
    UPtr(UPtr &&other) noexcept : UPtr() { Swap(other); }

    UPtr &operator=(const UPtr &) = delete;
    UPtr &operator=(UPtr &&other) noexcept { Swap(other); return *this; }

    ~UPtr() { delete ptr_; }

    void Swap(UPtr &other) noexcept { std::swap(ptr_, other.ptr_); }

    void Reset(T *ptr = nullptr) { UPtr t(ptr); Swap(t); }

    operator bool() const { return ptr_; }

    template<typename U> requires std::is_base_of_v<U, T>
    operator UPtr<U>() && { U *ptr = ptr_; ptr_ = nullptr; return UPtr<U>(ptr); }

    operator T *() const { return ptr_; }

    T *operator->() const { return ptr_; }

    T *Get() const { return ptr_; }

    T &operator*() const { return ptr_; }

    T *Release() { auto ret = ptr_; ptr_ = nullptr; return ret; }

    ReferenceCountedPtr<T> ToRC() { return ReferenceCountedPtr<T>(this->Release()); }

private:

    T *ptr_;
};

template<typename T, typename...Args>
UPtr<T> MakeUPtr(Args&&...args)
{
    T *object = new T(std::forward<Args>(args)...);
    return UPtr<T>(object);
}

template<typename A, typename B>
bool operator<(const UPtr<A> &a, const UPtr<B> &b)
{
    return a.Get() < b.Get();
}

template<typename A, typename B>
bool operator==(const UPtr<A> &a, const UPtr<B> &b)
{
    return a.Get() == b.Get();
}

template<typename A, typename B>
bool operator!=(const UPtr<A> &a, const UPtr<B> &b)
{
    return a.Get() != b.Get();
}

RTRC_END
