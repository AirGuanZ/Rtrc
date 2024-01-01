#pragma once

#include <Rtrc/Core/SmartPointer/ReferenceCounted.h>

RTRC_BEGIN

template<typename T>
concept Clonable = std::is_same_v<T *, decltype(std::declval<const T &>().Clone())>;

struct DefaultCopyOnWriteCloner
{
    template<typename T> requires Clonable<T>
    T *operator()(const T &other) const
    {
        return other.Clone();
    }

    template<typename T> requires !Clonable<T> &&std::is_copy_constructible_v<T>
    T *operator()(const T &other) const
    {
        return new T(other);
    }
};

template<typename T>
class CopyOnWritePtr
{
public:

    using ElementType = T;

    CopyOnWritePtr()
        : ptr_()
    {

    }

    CopyOnWritePtr(std::nullptr_t)
        : ptr_(nullptr)
    {

    }

    explicit CopyOnWritePtr(T *ptr)
        : ptr_(ptr)
    {

    }

    CopyOnWritePtr(const CopyOnWritePtr &other)
        : ptr_(other.ptr_)
    {

    }

    CopyOnWritePtr(CopyOnWritePtr &&other) noexcept
        : ptr_(std::move(other.ptr_))
    {

    }

    CopyOnWritePtr(const ReferenceCountedPtr<T> &other)
        : ptr_(other)
    {

    }

    CopyOnWritePtr(ReferenceCountedPtr<T> &&other) noexcept
        : ptr_(std::move(other))
    {

    }

    ~CopyOnWritePtr() = default;

    CopyOnWritePtr &operator=(const CopyOnWritePtr &other)
    {
        ptr_ = other.ptr_;
        return *this;
    }

    CopyOnWritePtr &operator=(CopyOnWritePtr &&other) noexcept
    {
        ptr_ = std::move(other.ptr_);
        return *this;
    }

    CopyOnWritePtr &operator=(const ReferenceCountedPtr<T> &other)
    {
        ptr_ = other;
        return *this;
    }

    CopyOnWritePtr &operator=(ReferenceCountedPtr<T> &&other) noexcept
    {
        ptr_ = std::move(other);
        return *this;
    }

    void Swap(CopyOnWritePtr &other) noexcept
    {
        ptr_.Swap(other.ptr_);
    }

    void Reset(T *ptr = nullptr)
    {
        ptr_.Reset(ptr);
    }

    operator bool() const
    {
        return ptr_;
    }

    bool IsUnique() const
    {
        return ptr_.GetCounter() == 1;
    }

    template<typename U> requires std::is_base_of_v<U, T>
    operator CopyOnWritePtr<U>() const
    {
        U *ptr = ptr_;
        return CopyOnWritePtr(ptr);
    }

    operator ReferenceCountedPtr<T>() const
    {
        return ptr_;
    }

    operator const T *() const
    {
        return ptr_.Get();
    }

    const T *operator->() const
    {
        return ptr_.Get();
    }

    const T &operator*() const
    {
        return *ptr_;
    }

    const T *Get() const
    {
        return ptr_.Get();
    }

    template<typename Cloner = DefaultCopyOnWriteCloner>
    T *Unshare(const Cloner &cloner = DefaultCopyOnWriteCloner{})
    {
        assert(ptr_);
        if(GetCounter() > 1)
        {
            ptr_ = ReferenceCountedPtr<T>(cloner(*ptr_));
            assert(GetCounter() == 1);
        }
        return ptr_.Get();
    }

    uint32_t GetCounter() const
    {
        return ptr_.GetCounter();
    }

private:

    ReferenceCountedPtr<T> ptr_;
};

RTRC_END
