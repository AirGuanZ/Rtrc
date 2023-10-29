#pragma once

#include <Core/SmartPointer/CopyOnWritePtr.h>
#include <Core/SmartPointer/UniquePointer.h>

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

    operator bool() const { return pointer_ != nullptr; }
    operator T *() const { return pointer_; }
    T &operator*() const { return *pointer_; }
    T *operator->() const { return pointer_; }
    T *Get() const { return pointer_; }
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

    operator bool() const { return pointer_ != nullptr; }
    operator const T *() const { return pointer_; }
    const T &operator*() const { return *pointer_; }
    const T *operator->() const { return pointer_; }
    const T *Get() const { return pointer_; }
};

RTRC_END
