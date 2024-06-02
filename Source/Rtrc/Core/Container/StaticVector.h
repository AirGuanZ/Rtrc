#pragma once

#include <cassert>

#include <Rtrc/Core/Hash.h>
#include <Rtrc/Core/ScopeGuard.h>

RTRC_BEGIN

template<typename T, size_t N>
class StaticVector
{
    T *RawAddr(size_t i)
    {
        return reinterpret_cast<T *>(&data_) + i;
    }

public:

    static constexpr size_t Capacity = N;

    StaticVector()
        : size_(0)
    {
        
    }

    StaticVector(std::initializer_list<T> initData)
    {
        assert(initData.size() <= Capacity);
        size_ = 0;
        RTRC_SCOPE_FAIL{ Destruct(); };
        while(size_ < initData.size())
        {
            new(RawAddr(size_)) T(std::data(initData)[size_]);
            ++size_;
        }
    }

    explicit StaticVector(size_t initialCount, const T &value = {})
    {
        assert(initialCount <= Capacity);
        size_ = 0;
        RTRC_SCOPE_FAIL{ Destruct(); };
        while(size_ < initialCount)
        {
            new(RawAddr(size_)) T(value);
            ++size_;
        }
    }

    StaticVector(const StaticVector &other)
    {
        size_ = 0;
        RTRC_SCOPE_FAIL{ Destruct(); };
        while(size_ < other.GetSize())
        {
            new(RawAddr(size_)) T(other.At(size_));
            ++size_;
        }
    }

    StaticVector(StaticVector &&other) noexcept
        : StaticVector()
    {
        this->Swap(other);
    }

    StaticVector(const T *begin, const T *end)
        : StaticVector()
    {
        while(begin < end)
        {
            this->PushBack(*begin++);
        }
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

        const size_t swappedCount = (std::min)(size_, other.size_);
        for(size_t i = 0; i < swappedCount; ++i)
        {
            std::swap(At(i), other.At(i));
        }

        if(size_ > other.size_)
        {
            for(size_t i = other.size_; i < size_; ++i)
            {
                new (other.RawAddr(i)) T(std::move(At(i)));
                At(i).~T();
            }
        }
        else if(size_ < other.size_)
        {
            for(size_t i = size_; i < other.size_; ++i)
            {
                new (RawAddr(i)) T(std::move(other.At(i)));
                other.At(i).~T();
            }
        }

        std::swap(size_, other.size_);
    }

    void Resize(size_t newSize)
    {
        this->ResizeImpl(newSize, [](T *addr) { new(addr) T(); });
    }

    void Resize(size_t newSize, const T &newValue)
    {
        this->ResizeImpl(newSize, [&newValue](T *addr) { new(addr) T(newValue); });
    }

    void PushBack(const T &value)
    {
        assert(size_ < Capacity);
        new (RawAddr(size_)) T(value);
        ++size_;
    }

    void PushBack(T &&value) noexcept
    {
        assert(size_ < Capacity);
        new (RawAddr(size_)) T(std::move(value));
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
        return *std::launder(reinterpret_cast<const T *>(&data_) + i);
    }

    T &At(size_t i)
    {
        assert(i < size_);
        return *std::launder(reinterpret_cast<T *>(&data_) + i);
    }

    size_t GetSize() const
    {
        return size_;
    }

    const T *GetData() const
    {
        return std::launder(reinterpret_cast<const T *>(&data_));
    }

    T *GetData()
    {
        return std::launder(reinterpret_cast<T *>(&data_));
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

    auto operator<=>(const StaticVector &rhs) const
    {
        return std::lexicographical_compare_three_way(begin(), end(), rhs.begin(), rhs.end());
    }

    bool operator==(const StaticVector &rhs) const
    {
        return std::equal(begin(), end(), rhs.begin(), rhs.end());
    }

    bool operator!=(const StaticVector &rhs) const
    {
        return !(*this == rhs);
    }

    size_t size() const { return GetSize(); }

    auto begin() { return std::launder(reinterpret_cast<T *>(&data_)); }
    auto end() { return begin() + size_; }

    auto begin() const { return std::launder(reinterpret_cast<const T *>(&data_)); }
    auto end() const { return begin() + size_; }

    bool empty() const { return IsEmpty(); }
    void push_back(const T &value) { this->PushBack(value); }
    void push_back(T &&value) { PushBack(std::move(value)); }
    void pop_back() { PopBack(); }
    auto &back() { return *(end() - 1); }
    auto &back() const { return *(end() - 1); }

    size_t Hash() const
    {
        return ::Rtrc::HashRange(this->begin(), this->end());
    }

private:

    using AlignedStorage = std::aligned_storage_t<sizeof(T) *N, alignof(T)>;

    template<typename Constructor>
    void ResizeImpl(size_t newSize, const Constructor &constructor)
    {
        if(newSize > size_)
        {
            const size_t oldSize = size_;
            try
            {
                while(size_ < newSize)
                {
                    constructor(RawAddr(size_));
                    ++size_;
                }
            }
            catch(...)
            {
                for(size_t i = oldSize; i < size_; ++i)
                {
                    At(i).~T();
                }
                throw;
            }
        }
        else // newSize <= size_
        {
            while(size_ > newSize)
            {
                At(size_ - 1).~T();
                --size_;
            }
        }
    }

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
