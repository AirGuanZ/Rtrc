#pragma once

#include <mimalloc.h>

#include <Rtrc/Utility/ScopeGuard.h>
#include <Rtrc/Utility/Uncopyable.h>

RTRC_BEGIN

class ObjectReleaser : public Uncopyable
{
    struct Destructor
    {
        Destructor *next = nullptr;
        virtual void Destruct() noexcept = 0;
        virtual ~Destructor() = default;
    };

    template<typename T>
    struct DestructorImpl : Destructor
    {
        T *ptr = nullptr;
        explicit DestructorImpl(T *ptr) noexcept: ptr(ptr) { }
        void Destruct() noexcept override { ptr->~T(); mi_free(ptr); }
    };
    
    Destructor *destructorEntry_ = nullptr;

    template<typename T>
    void AddDestructor(T *obj);

public:

    ObjectReleaser() = default;

    ~ObjectReleaser();

    template<typename T, typename...Args>
    T *Create(Args &&...args);

    void Destroy();
};

template<typename T>
void ObjectReleaser::AddDestructor(T *obj)
{
    if constexpr(std::is_trivially_destructible_v<T>)
        return;
    void *destructorMem = mi_aligned_alloc(alignof(DestructorImpl<T>), sizeof(DestructorImpl<T>));
    if(!destructorMem)
    {
        throw std::bad_alloc();
    }
    auto destructor = new(destructorMem) DestructorImpl<T>(obj);
    destructor->next = destructorEntry_;
    destructorEntry_ = destructor;
}

inline ObjectReleaser::~ObjectReleaser()
{
    Destroy();
}

template<typename T, typename...Args>
T *ObjectReleaser::Create(Args&&...args)
{
    void *objectMem = mi_aligned_alloc(alignof(T), sizeof(T));
    if(!objectMem)
    {
        throw std::bad_alloc();
    }
    RTRC_SCOPE_FAIL{ mi_free(objectMem); };
    T *obj = new(objectMem) T(std::forward<Args>(args)...);
    RTRC_SCOPE_FAIL{ obj->~T(); };
    this->AddDestructor(obj);
    return obj;
}

inline void ObjectReleaser::Destroy()
{
    for(Destructor *d = destructorEntry_, *nd; d; d = nd)
    {
        nd = d->next;
        d->Destruct();
        d->~Destructor();
        mi_free(d);
    }
    destructorEntry_ = nullptr;
}

RTRC_END
