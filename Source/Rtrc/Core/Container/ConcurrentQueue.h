#pragma once

#include <optional>

#include <Rtrc/Core/Common.h>
#include <Rtrc/Core/Container/moodycamel/concurrentqueue.h>

RTRC_BEGIN

// Note: Producers are assumed to be independent. For multiple producers, the elements may not come out in the same order they were put in.
// (for each individual producer, the input order is still kept)
template<typename T>
class ConcurrentQueue
{
public:

    void Push(T item);

    std::optional<T> TryPop();

    bool TryPop(T &item);

    size_t SizeApprox() const;

private:

    moodycamel::ConcurrentQueue<T> impl_;
};

template <typename T>
void ConcurrentQueue<T>::Push(T item)
{
    impl_.enqueue(std::move(item));
}

template <typename T>
std::optional<T> ConcurrentQueue<T>::TryPop()
{
    T item;
    if(impl_.try_dequeue(item))
    {
        return std::optional<T>(std::move(item));
    }
    return std::nullopt;
}

template <typename T>
bool ConcurrentQueue<T>::TryPop(T &item)
{
    return impl_.try_dequeue(item);
}

template <typename T>
size_t ConcurrentQueue<T>::SizeApprox() const
{
    return impl_.size_approx();
}

RTRC_END
