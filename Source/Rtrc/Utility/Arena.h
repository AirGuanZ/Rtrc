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

class LinearAllocator : public Uncopyable
{
    struct Destructor
    {
        Destructor *next = nullptr;
        virtual void Destruct() noexcept = 0;
        virtual ~Destructor() = default;
    };

    template<typename T>
    struct RegularDestructor : Destructor
    {
        T *ptr = nullptr;
        explicit RegularDestructor(T *ptr) noexcept : ptr(ptr) { }
        void Destruct() noexcept override { ptr->~T(); }
    };

    template<typename T>
    struct LargeDestructor : Destructor
    {
        T *ptr = nullptr;
        explicit LargeDestructor(T *ptr) noexcept : ptr(ptr) { }
        void Destruct() noexcept override;
    };

    struct ChunkHead
    {
        ChunkHead *next = nullptr;
    };

    size_t chunkSize_;
    size_t halfChunkSize_;

    Destructor *destructorEntry_;
    ChunkHead *chunkEntry_;

    unsigned char *currentPtr_;
    size_t restBytes_;

    template<typename T>
    T *AllocateSmallObject();

    void AllocateNewChunk();

public:

    explicit LinearAllocator(size_t chunkSize = 2 * 1024 * 1024);

    ~LinearAllocator();

    template<typename T, typename...Args>
    T *Create(Args&&...args);

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

template<typename T>
void LinearAllocator::LargeDestructor<T>::Destruct() noexcept
{
    ptr->~T();
    if(alignof(T) <= alignof(void *))
    {
        ::mi_free(ptr);
    }
    else
    {
        ::mi_free_aligned(ptr, alignof(T));
    }
}

template<typename T>
T *LinearAllocator::AllocateSmallObject()
{
    static_assert(sizeof(size_t) == sizeof(unsigned char *));
    unsigned char *alignedPtr = reinterpret_cast<unsigned char*>(
        (reinterpret_cast<std::size_t>(currentPtr_) + alignof(T) - 1) / alignof(T) * alignof(T));
    const size_t alignBytes = alignedPtr - currentPtr_;
    if(restBytes_ < alignBytes + sizeof(T))
    {
        AllocateNewChunk();
        return AllocateSmallObject<T>();
    }
    restBytes_ -= alignBytes + sizeof(T);
    currentPtr_ = alignedPtr + sizeof(T);
    return reinterpret_cast<T*>(alignedPtr);
}

inline void LinearAllocator::AllocateNewChunk()
{
    auto memory = static_cast<unsigned char*>(mi_malloc(chunkSize_));
    if(!memory)
    {
        throw Exception("LinearAllocator: failed to allocate new memory chunk");
    }

    auto chunkHead = reinterpret_cast<ChunkHead *>(memory);
    chunkHead->next = chunkEntry_;
    chunkEntry_ = chunkHead;

    currentPtr_ = memory + sizeof(ChunkHead);
    restBytes_ = chunkSize_ - sizeof(ChunkHead);
}

inline LinearAllocator::LinearAllocator(size_t chunkSize)
    : chunkSize_(chunkSize)
    , halfChunkSize_(chunkSize >> 1)
    , destructorEntry_(nullptr)
    , chunkEntry_(nullptr)
    , currentPtr_(nullptr)
    , restBytes_(0)
{
    assert(halfChunkSize_ >= sizeof(ChunkHead));
    assert(halfChunkSize_ >= sizeof(RegularDestructor<std::string>));
    assert(halfChunkSize_ >= sizeof(LargeDestructor<std::string>));
}

inline LinearAllocator::~LinearAllocator()
{
    Destroy();
}

template<typename T, typename... Args>
T *LinearAllocator::Create(Args &&... args)
{
    if(sizeof(T) > halfChunkSize_)
    {
        auto destructor = AllocateSmallObject<LargeDestructor<T>>();
        T *ret;
        if constexpr(alignof(T) <= alignof(void*))
        {
            ret = static_cast<T*>(mi_malloc(sizeof(T)));
        }
        else
        {
            ret = static_cast<T *>(mi_aligned_alloc(alignof(T), sizeof(T)));
        };
        new(ret) T(std::forward<Args>(args)...);
        new(destructor) LargeDestructor<T>(ret);
        destructor->next = destructorEntry_;
        destructorEntry_ = destructor;
        return ret;
    }

    if constexpr(std::is_trivially_destructible_v<T>)
    {
        T *ret = AllocateSmallObject<T>();
        new(ret) T(std::forward<Args>(args)...);
        return ret;
    }
    else
    {
        auto destructor = AllocateSmallObject<RegularDestructor<T>>();
        T *ret = AllocateSmallObject<T>();
        new(ret) T(std::forward<Args>(args)...);
        new(destructor) RegularDestructor<T>(ret);
        destructor->next = destructorEntry_;
        destructorEntry_ = destructor;
        return ret;
    }
}

inline void LinearAllocator::Destroy()
{
    while(destructorEntry_)
    {
        destructorEntry_->Destruct();
        destructorEntry_ = destructorEntry_->next;
    }
    while(chunkEntry_)
    {
        ChunkHead *nextChunk = chunkEntry_->next;
        mi_free(chunkEntry_);
        chunkEntry_ = nextChunk;
    }
    destructorEntry_ = nullptr;
    chunkEntry_ = nullptr;
    currentPtr_ = nullptr;
    restBytes_ = 0;
}

RTRC_END
