#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include <Core/Common.h>

RTRC_BEGIN

template<typename T>
class SlotVector
{
public:

    explicit SlotVector(size_t initialCapacity = 8);

    SlotVector(const SlotVector &other);
    SlotVector(SlotVector &&other) noexcept;

    SlotVector &operator=(const SlotVector &other);
    SlotVector &operator=(SlotVector &&other) noexcept;

    ~SlotVector();

    void Swap(SlotVector &other) noexcept;

    template<typename...Args>
    int New(Args &&...args);
    void Delete(int index);
    void Clear(bool freeMemory = false);

    const T &At(int index) const;
    T &At(int index);

    template<typename F>
    void ForEach(const F &f);

private:

    using ElementStorage = std::aligned_storage_t<sizeof(T), alignof(T)>;

    void Grow();

    std::vector<int> freeSlots_;
    std::vector<bool> isUsed_;
    std::vector<ElementStorage> values_;
};

template<typename T>
SlotVector<T>::SlotVector(size_t initialCapacity)
{
    if(initialCapacity > 0)
    {
        isUsed_.resize(initialCapacity, false);
        values_.resize(initialCapacity);
        freeSlots_.resize(initialCapacity);
        for(size_t i = 0; i < initialCapacity; ++i)
        {
            freeSlots_[i] = static_cast<int>(i);
        }
    }
}

template<typename T>
SlotVector<T>::SlotVector(const SlotVector &other)
{
    freeSlots_ = other.freeSlots_;
    isUsed_ = other.isUsed_;
    if constexpr(std::is_trivially_copy_constructible_v<T>)
    {
        values_ = other.values_;
    }
    else
    {
        values_.resize(other.values_);
        size_t i = 0;
        try
        {
            while(i < values_.size())
            {
                if(isUsed_[i])
                {
                    new(&values_[i]) T(other.At(i));
                }
                ++i;
            }
        }
        catch(...)
        {
            for(size_t j = 0; j < i; ++j)
            {
                if(isUsed_[j])
                {
                    At(j).~T();
                }
            }
        }
    }
}

template<typename T>
SlotVector<T>::SlotVector(SlotVector &&other) noexcept
    : SlotVector(0)
{
    this->Swap(other);
}

template<typename T>
SlotVector<T> &SlotVector<T>::operator=(const SlotVector &other)
{
    SlotVector t(other);
    this->Swap(t);
    return *this;
}

template<typename T>
SlotVector<T> &SlotVector<T>::operator=(SlotVector &&other) noexcept
{
    this->Swap(other);
    return *this;
}

template<typename T>
SlotVector<T>::~SlotVector()
{
    if constexpr(!std::is_trivially_destructible_v<T>)
    {
        for(size_t i = 0; i < isUsed_.size(); ++i)
        {
            if(isUsed_[i])
            {
                At(static_cast<int>(i)).~T();
            }
        }
    }
}

template<typename T>
void SlotVector<T>::Swap(SlotVector &other) noexcept
{
    freeSlots_.swap(other.freeSlots_);
    isUsed_.swap(other.isUsed_);
    values_.swap(other.values_);
}

template<typename T>
template<typename...Args>
int SlotVector<T>::New(Args &&...args)
{
    if(freeSlots_.empty())
    {
        Grow();
    }
    const int slot = freeSlots_.back();
    assert(!isUsed_[slot]);
    new(&values_[slot]) T(std::forward<Args>(args)...);
    isUsed_[slot] = true;
    freeSlots_.pop_back();
    return slot;
}

template<typename T>
void SlotVector<T>::Delete(int index)
{
    assert(isUsed_[index]);
    if constexpr(!std::is_trivially_destructible_v<T>)
    {
        At(index).~T();
    }
    freeSlots_.push_back(index);
    isUsed_[index] = false;
}

template<typename T>
void SlotVector<T>::Clear(bool freeMemory)
{
    if constexpr(!std::is_trivially_destructible_v<T>)
    {
        for(size_t i = 0; i < isUsed_.size(); ++i)
        {
            if(isUsed_[i])
            {
                At(i).~T();
                freeSlots_.push_back(static_cast<int>(i));
                isUsed_[i] = false;
            }
        }
    }

    if(freeMemory)
    {
        freeSlots_.clear();
        isUsed_.clear();
        values_.clear();
    }
}

template<typename T>
const T &SlotVector<T>::At(int index) const
{
    assert(isUsed_[index]);
    return *std::launder(reinterpret_cast<const T *>(&values_[index]));
}

template<typename T>
T &SlotVector<T>::At(int index)
{
    assert(isUsed_[index]);
    return *std::launder(reinterpret_cast<T *>(&values_[index]));
}

template<typename T>
template<typename F>
void SlotVector<T>::ForEach(const F &f)
{
    for(size_t i = 0; i < isUsed_.size(); ++i)
    {
        if(isUsed_[i])
        {
            f(At(static_cast<int>(i)));
        }
    }
}

template<typename T>
void SlotVector<T>::Grow()
{
    const size_t oldSize = values_.size();
    const size_t newSize = oldSize ? 2 * oldSize : 8;

    freeSlots_.reserve(freeSlots_.size() + (newSize - oldSize));
    isUsed_.resize(newSize, false);
    for(size_t i = oldSize; i < newSize; ++i)
    {
        freeSlots_.push_back(static_cast<int>(i));
    }

    if constexpr(std::is_trivially_move_constructible_v<T>)
    {
        values_.resize(newSize);
    }
    else
    {
        static_assert(std::is_nothrow_move_constructible_v<T>);
        std::vector<ElementStorage> newValues(newSize);
        for(size_t i = 0; i < oldSize; ++i)
        {
            if(isUsed_[i])
            {
                new(&newValues[i]) T(std::move(At(static_cast<int>(i))));
                At(static_cast<int>(i)).~T();
            }
        }
        values_ = std::move(newValues);
    }
}

RTRC_END
