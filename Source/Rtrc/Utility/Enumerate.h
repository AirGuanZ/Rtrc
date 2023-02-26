#pragma once

#include <tuple>

#include <Rtrc/Common.h>

RTRC_BEGIN

// from https://gist.github.com/yujincheng08/a65bb3ce21f8b91c0292d0974b8dee23
template <typename T>
class Enumerate
{
    struct iterator
    {
        using iter_type = decltype(std::begin(std::declval<T>()));

    private:

        friend class Enumerate;

        iterator(const std::size_t &i, iter_type iter)
            : i_(i), iter_(std::move(iter)) {}

        std::size_t i_;
        iter_type iter_;

    public:

        std::tuple<const std::size_t, typename std::iterator_traits<iter_type>::reference> operator*()
        {
            return { i_, *iter_ };
        }

        bool operator!=(const iterator &other)
        {
            return other.iter_ != iter_;
        }

        auto operator++()
        {
            ++i_;
            ++iter_;
            return *this;
        }
    };

public:

    Enumerate(const Enumerate &) = delete;

    Enumerate(Enumerate &&other) noexcept
        : container_(std::move(other.container_))
    {
        
    }

    template<typename U> requires !std::is_same_v<Enumerate, std::remove_reference_t<U>>
    explicit Enumerate(U &&container)
        : container_(std::forward<U>(container))
    {

    }

    auto begin()
    {
        return iterator{ 0, std::begin(container_) };
    }

    auto end()
    {
        return iterator{ Rtrc::GetContainerSize(container_), std::end(container_) };
    }

private:

    T container_;
};

template <typename U>
Enumerate(U &&container)
    ->Enumerate<std::enable_if_t<!std::is_rvalue_reference_v<decltype(std::forward<U>(container))>, U &>>;

template <typename U>
Enumerate(U &&container)
    ->Enumerate<std::enable_if_t<std::is_rvalue_reference_v<decltype(std::forward<U>(container))>, const U>>;

RTRC_END
