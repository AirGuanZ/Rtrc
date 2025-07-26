#pragma once

#include <vector>

#include <Rtrc/Core/Container/BitVector.h>
#include <Rtrc/Core/Container/Span.h>

RTRC_BEGIN

template<typename T>
class SparseArray
{
public:

    SparseArray() = default;
    SparseArray(const SparseArray &other);
    SparseArray(SparseArray &&other) noexcept;
    ~SparseArray();

    SparseArray &operator=(const SparseArray &other);
    SparseArray &operator=(SparseArray &&other) noexcept;

    void Swap(SparseArray &other) noexcept;

    template<typename...Args>
    size_t Allocate(Args&&...args);
    void Free(size_t index);

          T &operator[](size_t index);
    const T &operator[](size_t index) const;

    size_t GetSize() const;
    bool IsEmpty() const;

private:

    struct Storage
    {
        alignas(alignof(T)) unsigned char data[sizeof(T)];
    };

    static_assert(alignof(Storage) >= alignof(T));

    void ExpandStorage(size_t newCount);

          T *GetPointer(size_t index);
    const T *GetPointer(size_t index) const;

    std::vector<Storage> storageArray_;
    std::vector<size_t> freeIndices_;
    BitVector<uint32_t> occupiedBits_;
};

template <typename T>
SparseArray<T>::SparseArray(const SparseArray &other)
{
    assert(other.storageArray_.size() == other.occupiedBits_.GetSize());

    storageArray_.resize(other.storageArray_.size());
    freeIndices_ = other.freeIndices_;
    occupiedBits_ = other.occupiedBits_;

    size_t constructedElementCount = 0;
    try
    {
        for(size_t i = 0; i < other.storageArray_.size(); ++i)
        {
            if(other.occupiedBits_[i])
            {
                new (reinterpret_cast<T *>(storageArray_[i].data)) T(*other.GetPointer(i));
                ++constructedElementCount;
            }
        }
    }
    catch(...)
    {
        for(size_t i = other.storageArray_.size(); i > 0 && constructedElementCount > 0; --i)
        {
            const size_t idx = i - 1;
            if(other.occupiedBits_[idx])
            {
                std::destroy_at(GetPointer(idx));
                --constructedElementCount;
            }
        }
        throw;
    }
}

template <typename T>
SparseArray<T>::SparseArray(SparseArray &&other) noexcept
    : SparseArray()
{
    this->Swap(other);
}

template <typename T>
SparseArray<T>::~SparseArray()
{
    if constexpr(std::is_trivially_destructible_v<T>)
    {
        return;
    }

    if(freeIndices_.size() == storageArray_.size())
    {
        return;
    }

    for(size_t i = 0; i < storageArray_.size(); ++i)
    {
        if(occupiedBits_[i])
        {
            std::destroy_at(GetPointer(i));
        }
    }
}

template <typename T>
SparseArray<T> &SparseArray<T>::operator=(const SparseArray &other)
{
    SparseArray t(other);
    this->Swap(t);
    return *this;
}

template <typename T>
SparseArray<T> &SparseArray<T>::operator=(SparseArray &&other) noexcept
{
    this->Swap(other);
    return *this;
}

template <typename T>
void SparseArray<T>::Swap(SparseArray &other) noexcept
{
    storageArray_.swap(other.storageArray_);
    freeIndices_.swap(other.freeIndices_);
    occupiedBits_.Swap(other.occupiedBits_);
}

template <typename T>
template <typename... Args>
size_t SparseArray<T>::Allocate(Args &&... args)
{
    if(freeIndices_.empty())
    {
        this->ExpandStorage((std::max<size_t>)(storageArray_.size() * 2, 16));
    }

    const size_t index = freeIndices_.back();
    freeIndices_.pop_back();

    try
    {
        new(GetPointer(index)) T(std::forward<Args>(args)...);
    }
    catch(...)
    {
        freeIndices_.push_back(index);
        throw;
    }

    assert(!occupiedBits_[index]);
    occupiedBits_[index] = true;
    return index;
}

template <typename T>
void SparseArray<T>::Free(size_t index)
{
    assert(index < storageArray_.size());
    assert(occupiedBits_[index]);
    std::destroy_at(GetPointer(index));
    freeIndices_.push_back(index);
    occupiedBits_[index] = false;
}

template <typename T>
T &SparseArray<T>::operator[](size_t index)
{
    assert(index < storageArray_.size());
    assert(occupiedBits_[index]);
    return *GetPointer(index);
}

template <typename T>
const T &SparseArray<T>::operator[](size_t index) const
{
    assert(index < storageArray_.size());
    assert(occupiedBits_[index]);
    return *GetPointer(index);
}

template <typename T>
size_t SparseArray<T>::GetSize() const
{
    return storageArray_.size() - freeIndices_.size();
}

template <typename T>
bool SparseArray<T>::IsEmpty() const
{
    return freeIndices_.size() == storageArray_.size();
}

template <typename T>
void SparseArray<T>::ExpandStorage(size_t newCount)
{
    const size_t oldCount = storageArray_.size();
    assert(newCount > oldCount);

    freeIndices_.reserve(freeIndices_.size() + newCount - oldCount);
    occupiedBits_.Reserve(newCount);

    if constexpr(std::is_trivially_move_constructible_v<T>)
    {
        storageArray_.resize(newCount);
    }
    else
    {
        static_assert(std::is_nothrow_move_constructible_v<T>);
        std::vector<Storage> newStorage(newCount);
        for(size_t i = 0; i < oldCount; ++i)
        {
            if(occupiedBits_[i])
            {
                T *dst = reinterpret_cast<T *>(newStorage[i].data);
                new(dst) T(std::move(*GetPointer(i)));
            }
        }
        for(size_t i = 0; i < oldCount; ++i)
        {
            if(occupiedBits_[i])
            {
                std::destroy_at(GetPointer(i));
            }
        }
        storageArray_.swap(newStorage);
    }

    occupiedBits_.Resize(newCount);
    for(size_t i = oldCount; i < newCount; ++i)
    {
        freeIndices_.push_back(i);
    }
}

template <typename T>
T *SparseArray<T>::GetPointer(size_t index)
{
    return std::launder(reinterpret_cast<T*>(storageArray_[index].data));
}

template <typename T>
const T *SparseArray<T>::GetPointer(size_t index) const
{
    return std::launder(reinterpret_cast<const T *>(storageArray_[index].data));
}

RTRC_END
