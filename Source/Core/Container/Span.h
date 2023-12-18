#pragma once

#include <cassert>
#include <vector>

#include <Core/Container/StaticVector.h>

RTRC_BEGIN

template<typename T>
class MutSpan
{
public:

    using ElementType = T;

    MutSpan() = default;

    MutSpan(T &value) : MutSpan(&value, 1) { }
    
    MutSpan(T *data, uint32_t count) : data_(data), size_(count) { }

    MutSpan(T *begin, T *end) : MutSpan(begin, end - begin) { }

    MutSpan(std::vector<T> &data) : MutSpan(data.data(), static_cast<uint32_t>(data.size())) { }

    template<size_t N>
    MutSpan(StaticVector<T, N> &data) : MutSpan(data.GetData(), data.GetSize()) { }

    template<size_t N>
    MutSpan(T(&data)[N]) : MutSpan(data, N) { }

    bool IsEmpty() const { return size_ == 0; }

    T *GetData() { return data_; }

    uint32_t GetSize() const { return size_; }

    T &operator[](size_t i) { assert(i < size_); return data_[i]; }

    MutSpan GetSubSpan(size_t offset, size_t count)
    {
        assert(offset + count <= size_);
        const size_t beg = offset;
        const size_t end = (std::min<size_t>)(beg + count, size_);
        if(end == beg)
        {
            return {};
        }
        return Span(data_ + beg, end - beg);
    }

    MutSpan GetSubSpan(size_t offset)
    {
        assert(offset <= size_);
        return GetSubSpan(offset, size_ - offset);
    }

    auto size() const { return size_; }
    auto begin() { return data_; }
    auto end() { return data_ + size_; }

    size_t Hash() const
    {
        if(IsEmpty())
        {
            return 0;
        }
        size_t ret = ::Rtrc::Hash(data_[0]);
        for(uint32_t i = 1; i < size_; ++i)
        {
            ret = ::Rtrc::HashCombine(ret, ::Rtrc::Hash(data_[i]));
        }
        return ret;
    }

private:

    T *data_ = nullptr;
    uint32_t size_ = 0;
};

template<typename T>
class Span
{
public:

    using ElementType = T;

    Span() = default;

    Span(const T &value) : Span(&value, 1) { }

    Span(std::initializer_list<T> initList) : Span(initList.begin(), initList.size()) { }

    Span(const T *data, size_t count) : data_(data), size_(count) { }

    Span(const T *begin, const T *end) : Span(begin, end - begin) { }

    Span(const std::vector<T> &data) : Span(data.data(), data.size()) { }

    template<size_t N>
    Span(const StaticVector<T, N> &data) : Span(data.GetData(), data.GetSize()) { }

    template<size_t N>
    Span(const T(&data)[N]) : Span(data, N) { }

    Span(MutSpan<T> span) : Span(span.GetData(), span.GetSize()) { }

    bool IsEmpty() const { return size_ == 0; }

    const T *GetData() const { return data_; }

    uint32_t GetSize() const { return size_; }

    const T &operator[](size_t i) const { assert(i < size_); return data_[i]; }

    Span GetSubSpan(size_t offset, size_t count)
    {
        assert(offset + count <= size_);
        const size_t beg = offset;
        const size_t end = (std::min<size_t>)(beg + count, size_);
        if(end == beg)
        {
            return {};
        }
        return Span(data_ + beg, end - beg);
    }

    Span GetSubSpan(size_t offset)
    {
        assert(offset <= size_);
        return GetSubSpan(offset, size_ - offset);
    }

    auto size() const { return size_; }
    auto begin() const { return data_; }
    auto end() const { return data_ + size_; }

    size_t Hash() const
    {
        if(IsEmpty())
        {
            return 0;
        }
        size_t ret = ::Rtrc::Hash(data_[0]);
        for(uint32_t i = 1; i < size_; ++i)
        {
            ret = ::Rtrc::HashCombine(ret, ::Rtrc::Hash(data_[i]));
        }
        return ret;
    }

private:

    const T *data_ = nullptr;
    uint32_t size_ = 0;
};

template<typename T, uint32_t D>
class MultiDimMutSpan
{
public:

    using ElementType = T;
    static constexpr size_t Dimension = D;

    MultiDimMutSpan() = default;

    template<typename...TIs> requires (sizeof...(TIs) == D) && std::conjunction_v<std::is_integral<TIs>...>
    explicit MultiDimMutSpan(MutSpan<T> data, TIs...tis)
        : data_(data), sizes_{ static_cast<uint32_t>(tis)... }
    {
        assert(data_.GetSize() == [&]
        {
            uint32_t prod = 1;
            for(auto s : sizes_)
            {
                prod *= s;
            }
            return prod;
        }());
    }

    MultiDimMutSpan(MutSpan<T> data, Span<uint32_t> sizes)
        : data_(data), sizes_(sizes)
    {
        
    }

    bool IsEmpty() const
    {
        return data_.IsEmpty();
    }

    T *GetData() { return data_.GetData(); }

    uint32_t GetSize(size_t dim) const { return sizes_[dim]; }

    template<typename...TIs> requires (sizeof...(TIs) == D) && std::conjunction_v<std::is_integral<TIs>...>
    T &operator()(TIs...indices)
    {
        return data_[this->ComputeLinearIndex(indices...)];
    }

    auto size() const { return data_.size(); }
    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }

    size_t Hash() const
    {
        return data_.Hash();
    }

private:

    template<uint32_t DI>
    uint32_t ComputeLinearIndexAux(uint32_t accum, uint32_t a) const
    {
        static_assert(DI < D);
        return accum * sizes_[DI] + a;
    }

    template<uint32_t DI, typename...TIs>
    uint32_t ComputeLinearIndexAux(uint32_t accum, uint32_t a, uint32_t b, TIs...indices) const
    {
        static_assert(DI < D);
        accum = accum * sizes_[DI] + a;
        return this->template ComputeLinearIndexAux<DI + 1>(accum, b, indices...);
    }

    template<typename...TIs> requires (sizeof...(TIs) == D) && std::conjunction_v<std::is_integral<TIs>...>
    uint32_t ComputeLinearIndex(TIs...indices) const
    {
        return this->template ComputeLinearIndexAux<0>(0, indices...);
    }

    MutSpan<T> data_;
    std::array<uint32_t, D> sizes_ = { };
};

template<typename T, uint32_t D>
class MultiDimSpan
{
public:

    using ElementType = T;
    static constexpr size_t Dimension = D;

    MultiDimSpan() = default;

    template<typename...TIs> requires (sizeof...(TIs) == D) && std::conjunction_v<std::is_integral<TIs>...>
    explicit MultiDimSpan(Span<T> data, TIs...tis)
        : data_(data), sizes_{ static_cast<uint32_t>(tis)... }
    {
        assert(data_.GetSize() == [&]
            {
                uint32_t prod = 1;
                for(auto s : sizes_)
                {
                    prod *= s;
                }
                return prod;
            });
    }

    MultiDimSpan(Span<T> data, Span<uint32_t> sizes)
        : data_(data), sizes_(sizes)
    {

    }

    bool IsEmpty() const
    {
        return data_.IsEmpty();
    }

    const T *GetData() const { return data_.GetData(); }

    uint32_t GetSize(size_t dim) const { return sizes_[dim]; }

    template<typename...TIs> requires (sizeof...(TIs) == D) && std::conjunction_v<std::is_integral<TIs>...>
    const T &operator()(TIs...indices)
    {
        return data_[this->ComputeLinearIndex(indices...)];
    }

    auto size() const { return data_.size(); }
    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }

    size_t Hash() const
    {
        return data_.Hash();
    }

private:

    template<typename...TIs> requires (sizeof...(TIs) == D) && std::conjunction_v<std::is_integral<TIs>...>
    uint32_t ComputeLinearIndex(TIs...indices) const
    {
        return this->template ComputeLinearIndexAux<0>(0, indices...);
    }

    template<uint32_t DI>
    uint32_t ComputeLinearIndexAux(uint32_t accum, uint32_t a) const
    {
        static_assert(DI < D);
        return accum * sizes_[DI] + a;
    }

    template<uint32_t DI, typename...TIs> requires (sizeof...(TIs) == D) && std::conjunction_v<std::is_integral<TIs>...>
    uint32_t ComputeLinearIndexAux(uint32_t accum, uint32_t a, uint32_t b, TIs...indices) const
    {
        static_assert(DI < D);
        accum = accum * sizes_[DI] + a;
        return this->template ComputeLinearIndexAux<DI + 1>(accum, b, indices...);
    }

    Span<T> data_;
    std::array<uint32_t, D> sizes_ = { };
};

RTRC_END
