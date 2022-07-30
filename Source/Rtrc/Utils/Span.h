#pragma once

#include <cassert>
#include <vector>

#include <Rtrc/Common.h>

RTRC_BEGIN

template<typename T>
class Span
{
public:

    using ElementType = T;

    Span() : Span(nullptr, 0u) { }

    Span(const T &value) : Span(&value, 1) { }

    Span(std::initializer_list<T> initList) : Span(initList.begin(), initList.size()) { }

    Span(const T *data, uint32_t count) : data_(data), size_(count) { }

    Span(const T *begin, const T *end) : Span(begin, end - begin) { }

    Span(const std::vector<T> &data) : Span(data.data(), data.size()) { }

    bool IsEmpty() const { return size_ == 0; }

    const T *GetData() const { return data_; }

    uint32_t GetSize() const { return size_; }

    const T &operator[](size_t i) { assert(i < size_); return data_[i]; }

    auto size() const { return size_; }
    auto begin() const { return data_; }
    auto end() const { return data_ + size_; }

private:

    const T *data_;
    uint32_t size_;
};

RTRC_END
