#pragma once

#include <vector>

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

    // freeIndices must be sorted
    // func: return true to continue; return false to stop the traversal
    template<typename Func>
    static void ForEachNonFreeElementIndex(size_t indexCount, const std::vector<size_t> &freeIndices, const Func &func);

    std::vector<Storage> storageArray_;
    std::vector<size_t> freeIndices_;
};

template <typename T>
SparseArray<T>::SparseArray(const SparseArray &other)
{
    storageArray_.resize(other.storageArray_.size());

    freeIndices_ = other.freeIndices_;
    std::ranges::sort(freeIndices_);

    size_t constructedElementCount = 0;
    try
    {
        SparseArray::ForEachNonFreeElementIndex(other.storageArray_.size(), freeIndices_, [&](size_t i)
        {
            new (reinterpret_cast<T *>(storageArray_[i].data)) T(*other.GetPointer(i));
            ++constructedElementCount;
        });
    }
    catch(...)
    {
        SparseArray::ForEachNonFreeElementIndex(other.storageArray_.size(), freeIndices_, [&](size_t i)
        {
            if(constructedElementCount == 0)
            {
                return false;
            }
            std::destroy_at(GetPointer(i));
            --constructedElementCount;
            return true;
        });
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

    std::ranges::sort(freeIndices_);
    SparseArray::ForEachNonFreeElementIndex(storageArray_.size(), freeIndices_, [&](size_t i)
    {
        std::destroy_at(GetPointer(i));
        return true;
    });
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
    return index;
}

template <typename T>
void SparseArray<T>::Free(size_t index)
{
    std::destroy_at(GetPointer(index));
    freeIndices_.push_back(index);
}

template <typename T>
T &SparseArray<T>::operator[](size_t index)
{
    return *GetPointer(index);
}

template <typename T>
const T &SparseArray<T>::operator[](size_t index) const
{
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
    assert(newCount > storageArray_.size());

    freeIndices_.reserve(freeIndices_.size() + newCount - storageArray_.size());

    if constexpr(std::is_trivially_move_constructible_v<T>)
    {
        storageArray_.resize(newCount);
    }
    else
    {
        static_assert(std::is_nothrow_move_constructible_v<T>);
        std::vector<Storage> newStorage(newCount);
        for(size_t i = 0; i < storageArray_.size(); ++i)
        {
            T *dst = reinterpret_cast<T *>(newStorage[i].data);
            new(dst) T(std::move(*GetPointer(i)));
        }
        storageArray_.swap(newStorage);
    }

    for(size_t i = storageArray_.size(); i < newCount; ++i)
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

template <typename T>
template <typename Func>
void SparseArray<T>::ForEachNonFreeElementIndex(
    size_t indexCount, const std::vector<size_t> &freeIndices, const Func &func)
{
    size_t i = 0, j = 0;
    while(i < indexCount && j < freeIndices.size())
    {
        if(freeIndices[j] != i)
        {
            if(!func(i))
            {
                return;
            }
            ++j;
        }
        ++i;
    }

    assert(j == freeIndices.size());
    if(i < indexCount)
    {
        if(!func(i))
        {
            return;
        }
        ++i;
    }
}

RTRC_END
