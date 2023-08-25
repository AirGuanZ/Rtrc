#pragma once

#include <cassert>
#include <vector>

#include <Rtrc/Core/Container/StaticVector.h>

RTRC_BEGIN

template<typename T>
class Span
{
public:

    using ElementType = T;

    Span() = default;

    Span(const T &value) : Span(&value, 1) { }

    Span(std::initializer_list<T> initList) : Span(initList.begin(), initList.size()) { }

    Span(const T *data, uint32_t count) : data_(data), size_(count) { }

    Span(const T *begin, const T *end) : Span(begin, end - begin) { }

    Span(const std::vector<T> &data) : Span(data.data(), static_cast<uint32_t>(data.size())) { }

    template<size_t N>
    Span(const StaticVector<T, N> &data) : Span(data.GetData(), data.GetSize()) { }

    template<size_t N>
    Span(const T(&data)[N]) : Span(data, N) { }

    bool IsEmpty() const { return size_ == 0; }

    const T *GetData() const { return data_; }

    uint32_t GetSize() const { return size_; }

    const T &operator[](size_t i) const { assert(i < size_); return data_[i]; }

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

template<typename T>
class MutableSpan
{
public:

    using ElementType = T;

    MutableSpan() = default;

    MutableSpan(T &value) : MutableSpan(&value, 1) { }
    
    MutableSpan(T *data, uint32_t count) : data_(data), size_(count) { }

    MutableSpan(T *begin, T *end) : MutableSpan(begin, end - begin) { }

    MutableSpan(std::vector<T> &data) : MutableSpan(data.data(), static_cast<uint32_t>(data.size())) { }

    template<size_t N>
    MutableSpan(StaticVector<T, N> &data) : MutableSpan(data.GetData(), data.GetSize()) { }

    template<size_t N>
    MutableSpan(T(&data)[N]) : MutableSpan(data, N) { }

    bool IsEmpty() const { return size_ == 0; }

    T *GetData() { return data_; }

    uint32_t GetSize() const { return size_; }

    T &operator[](size_t i) { assert(i < size_); return data_[i]; }

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

RTRC_END
