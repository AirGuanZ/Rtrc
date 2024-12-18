#pragma once

#include <Rtrc/Core/SmartPointer/CopyOnWritePtr.h>
#include <Rtrc/Core/SmartPointer/UniquePointer.h>

RTRC_BEGIN

template<typename T>
class ObserverPtr
{
    T *pointer_;

public:

    using ElementType = T;

    ObserverPtr() : pointer_(nullptr) { }
    ObserverPtr(T *pointer) : pointer_(pointer) { }
    ObserverPtr(T &data) : pointer_(&data) { }
    ObserverPtr(const Box<T> &pointer) : ObserverPtr(pointer.get()) { }
    ObserverPtr(const RC<T> &pointer) : ObserverPtr(pointer.get()) { }
    ObserverPtr(const UPtr<T> &pointer) : ObserverPtr(pointer.Get()) { }
    ObserverPtr(const ReferenceCountedPtr<T> &pointer) : ObserverPtr(pointer.Get()) { }
    ObserverPtr(const CopyOnWritePtr<T> &pointer) : ObserverPtr(pointer.Get()) { }
    ObserverPtr(std::nullptr_t) : pointer_(nullptr) { }

    void Swap(ObserverPtr &other) noexcept { std::swap(pointer_, other.pointer_); }

    operator bool() const { return pointer_ != nullptr; }
    operator T *() const { return pointer_; }
    T &operator*() const { return *pointer_; }
    T *operator->() const { return pointer_; }
    T *Get() const { return pointer_; }

    auto operator<=>(const ObserverPtr &) const = default;
};

template<typename T>
class ObserverPtr<const T>
{
    const T *pointer_;

public:

    using ElementType = const T;

    ObserverPtr() : pointer_(nullptr) { }
    ObserverPtr(const T *pointer) : pointer_(pointer) { }
    ObserverPtr(const T &data) : pointer_(&data) { }
    ObserverPtr(const Box<T> &pointer) : ObserverPtr(pointer.get()) { }
    ObserverPtr(const Box<const T> &pointer) : ObserverPtr(pointer.get()) { }
    ObserverPtr(const RC<T> &pointer) : ObserverPtr(pointer.get()) { }
    ObserverPtr(const RC<const T> &pointer) : ObserverPtr(pointer.get()) { }
    ObserverPtr(const UPtr<T> &pointer) : ObserverPtr(pointer.Get()) { }
    ObserverPtr(const UPtr<const T> &pointer) : ObserverPtr(pointer.Get()) { }
    ObserverPtr(const ReferenceCountedPtr<T> &pointer) : ObserverPtr(pointer.Get()) { }
    ObserverPtr(const ReferenceCountedPtr<const T> &pointer) : ObserverPtr(pointer.Get()) { }
    ObserverPtr(const CopyOnWritePtr<T> &pointer) : ObserverPtr(pointer.Get()) { }
    ObserverPtr(const CopyOnWritePtr<const T> &pointer) : ObserverPtr(pointer.Get()) { }
    ObserverPtr(ObserverPtr<T> pointer) : pointer_(pointer.Get()) { }
    ObserverPtr(std::nullptr_t) : pointer_(nullptr) { }

    void Swap(ObserverPtr &other) noexcept { std::swap(pointer_, other.pointer_); }

    operator bool() const { return pointer_ != nullptr; }
    operator const T *() const { return pointer_; }
    const T &operator*() const { return *pointer_; }
    const T *operator->() const { return pointer_; }
    const T *Get() const { return pointer_; }

    auto operator<=>(const ObserverPtr &) const = default;
};

template<typename T>
bool operator==(const ObserverPtr<T> &lhs, std::nullptr_t)
{
    return lhs.Get() == nullptr;
}

template<typename T>
bool operator==(std::nullptr_t, const ObserverPtr<T> &ptr)
{
    return ptr.Get() == nullptr;
}

template<typename T>
bool operator!=(const ObserverPtr<T> &lhs, std::nullptr_t)
{
    return lhs.Get() != nullptr;
}

template<typename T>
bool operator!=(std::nullptr_t, const ObserverPtr<T> &ptr)
{
    return ptr.Get() != nullptr;
}

template<typename U, typename T> requires std::is_base_of_v<T, U> || std::is_same_v<T, U>
ObserverPtr<U> DynamicCast(const ObserverPtr<T> &ptr)
{
    auto castedPtr = dynamic_cast<U *>(ptr.Get());
    return ObserverPtr<U>(castedPtr);
}

template<typename T>
using Ref = ObserverPtr<T>;

RTRC_END
