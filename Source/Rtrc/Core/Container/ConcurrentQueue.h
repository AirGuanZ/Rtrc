#pragma once

#include <optional>

#include <Rtrc/Core/Uncopyable.h>
#include <Rtrc/Core/Container/moodycamel/ConcurrentQueue.h>
#include <Rtrc/Core/Container/moodycamel/blockingconcurrentqueue.h>

RTRC_BEGIN

template<typename T>
class ConcurrentQueueBlocking : public Uncopyable
{
public:

    void Push(T item);

    std::optional<T> TryPop();

    bool TryPop(T &item);

    void WaitToPop(T &item);

private:

    moodycamel::BlockingConcurrentQueue<T> impl_;
};

// Note: Producers are assumed to be independent. For multiple producers, the elements may not come out in the same order they were put in.
// (for each individual producer, the input order is still kept)
template<typename T>
class ConcurrentQueueNonBlocking : public Uncopyable
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
void ConcurrentQueueBlocking<T>::Push(T item)
{
    impl_.enqueue(std::move(item));
}

template <typename T>
std::optional<T> ConcurrentQueueBlocking<T>::TryPop()
{
    T item;
    if(impl_.try_dequeue(item))
    {
        return std::make_optional(std::move(item));
    }
    return std::nullopt;
}

template <typename T>
bool ConcurrentQueueBlocking<T>::TryPop(T &item)
{
    return impl_.try_dequeue(item);
}

template <typename T>
void ConcurrentQueueBlocking<T>::WaitToPop(T &item)
{
    impl_.wait_dequeue(item);
}

template <typename T>
void ConcurrentQueueNonBlocking<T>::Push(T item)
{
    impl_.enqueue(std::move(item));
}

template <typename T>
std::optional<T> ConcurrentQueueNonBlocking<T>::TryPop()
{
    T item;
    if(impl_.try_dequeue(item))
    {
        return std::optional<T>(std::move(item));
    }
    return std::nullopt;
}

template <typename T>
bool ConcurrentQueueNonBlocking<T>::TryPop(T &item)
{
    return impl_.try_dequeue(item);
}

template <typename T>
size_t ConcurrentQueueNonBlocking<T>::SizeApprox() const
{
    return impl_.size_approx();
}

RTRC_END
