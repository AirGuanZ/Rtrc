#pragma once

#include <Rtrc/Utility/SmartPointer/CopyOnWritePtr.h>

RTRC_BEGIN

template<typename T>
class ObserverPtr
{
    T *pointer_;

public:

    using ElementType = T;

    ObserverPtr(T *pointer) : pointer_(pointer) { }
    ObserverPtr(T &data) : pointer_(&data) { }
    ObserverPtr(const Box<T> &pointer) : ObserverPtr(pointer.get()) { }
    ObserverPtr(const RC<T> &pointer) : ObserverPtr(pointer.get()) { }
    ObserverPtr(const ReferenceCountedPtr<T> &pointer) : ObserverPtr(pointer.Get()) { }
    ObserverPtr(const CopyOnWritePtr<T> &pointer) : ObserverPtr(pointer.Get()) { }

    operator bool() const { return pointer_ != nullptr; }
    operator T *() const { return pointer_; }
    T &operator*() const { return *pointer_; }
    T *operator->() const { return pointer_; }
    T *Get() const { return pointer_; }
};

RTRC_END
