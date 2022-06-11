#pragma once

#include <cassert>

#include <Rtrc/Utils/ScopeGuard.h>

RTRC_BEGIN

template<typename T, size_t N>
class StaticVector
{
public:

    static constexpr size_t Capacity = N;

    StaticVector()
        : size_(0)
    {
        
    }

    explicit StaticVector(size_t initialCount, const T &value = {})
    {
        assert(initialCount < Capacity);
        size_ = 0;
        RTRC_SCOPE_FAIL{ Destruct(); };
        while(size_ < initialCount)
        {
            new(&At(size_)) T(value);
            ++size_;
        }
    }

    StaticVector(const StaticVector &other)
    {
        size_ = 0;
        RTRC_SCOPE_FAIL{ Destruct(); };
        while(size_ < other.GetSize())
        {
            new(&At(size_)) T(other.At(size_));
            ++size_;
        }
    }

    StaticVector(StaticVector &&other) noexcept
        : StaticVector()
    {
        this->Swap(other);
    }

    StaticVector &operator=(const StaticVector &other)
    {
        StaticVector temp(other);
        this->Swap(temp);
        return *this;
    }

    StaticVector &operator=(StaticVector &&other) noexcept
    {
        this->Swap(other);
        return *this;
    }

    ~StaticVector()
    {
        Destruct();
    }

    void Swap(StaticVector &other) noexcept
    {
        static_assert(noexcept(std::swap(std::declval<T &>(), std::declval<T &>())));
        static_assert(std::is_nothrow_move_constructible_v<T>);

        const size_t swappedCount = std::min(size_, other.size_);
        for(size_t i = 0; i < swappedCount; ++i)
        {
            std::swap(At(i), other.At(i));
        }

        if(size_ > other.size_)
        {
            for(size_t i = other.size_; i < size_; ++i)
            {
                new (&other.At(i)) T(std::move(At(i)));
                At(i).~T();
            }
        }
        else if(size_ < other.size_)
        {
            for(size_t i = size_; i < other.size_; ++i)
            {
                new (&At(i)) T(std::move(other.At(i)));
                other.At(i).~T();
            }
        }

        std::swap(size_, other.size_);
    }

    void PushBack(const T &value)
    {
        assert(size_ < Capacity);
        new (&At(size_)) T(value);
        ++size_;
    }

    void PushBack(T &&value) noexcept
    {
        assert(size_ < Capacity);
        new (&At(size_)) T(std::move(value));
        ++size_;
    }

    void PopBack()
    {
        assert(!IsEmpty());
        At(size_ - 1).~T();
        --size_;
    }

    const T &operator[](size_t i) const
    {
        return At(i);
    }

    T &operator[](size_t i)
    {
        return At(i);
    }

    const T &At(size_t i) const
    {
        assert(i < size_);
        return *(reinterpret_cast<const T *>(&data_) + i);
    }

    T &At(size_t i)
    {
        assert(i < size_);
        return *(reinterpret_cast<T *>(&data_) + i);
    }

    size_t GetSize() const
    {
        return size_;
    }

    const T *GetData() const
    {
        return reinterpret_cast<const T *>(&data_);
    }

    T *GetData()
    {
        return reinterpret_cast<T *>(&data_);
    }

    bool IsEmpty() const
    {
        return size_ == 0;
    }

    bool IsFull() const
    {
        return size_ == Capacity;
    }

    bool Contains(const T &val) const
    {
        return std::find(begin(), end(), val) != end();
    }

    auto begin() { return reinterpret_cast<T *>(&data_); }
    auto end() { return begin() + size_; }

    auto begin() const { return reinterpret_cast<const T *>(&data_); }
    auto end() const { return begin() + size_; }

private:

    using AlignedStorage = std::aligned_storage_t<sizeof(T) *N, alignof(T)>;

    void Destruct()
    {
        for(size_t j = 0; j < size_; ++j)
        {
            At(j).~T();
        }
    }

    size_t size_;
    AlignedStorage data_;
};

RTRC_END
