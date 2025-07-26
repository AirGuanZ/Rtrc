#pragma once

#include <cassert>

#include <Rtrc/Core/Uncopyable.h>

RTRC_BEGIN

template<typename T>
class IntrusiveList;

template<typename T>
class IntrusiveListNode
{
    friend class IntrusiveList<T>;

    IntrusiveListNode *prev_ = nullptr;
    IntrusiveListNode *succ_ = nullptr;
};

template<typename T>
class IntrusiveListIterator
{
    friend class IntrusiveList<T>;

    IntrusiveListNode<T> *node_ = nullptr;

public:

    T &operator*() const;

    IntrusiveListIterator &operator++();
    IntrusiveListIterator operator++(int);

    IntrusiveListIterator &operator--();
    IntrusiveListIterator operator--(int);

    bool operator==(const IntrusiveListIterator &other) const;
};

template<typename T>
class IntrusiveList : public Uncopyable
{
public:

    IntrusiveList();
    ~IntrusiveList();

    IntrusiveList(IntrusiveList &&other) noexcept;
    IntrusiveList &operator=(IntrusiveList &&other) noexcept;

    void Swap(IntrusiveList &other) noexcept;

    size_t GetSize() const;
    bool IsEmpty() const;
    void Clear();

    T &GetBack() const;
    T &GetFront() const;

    void PushBack(T &element);
    void PopBack();

    void PushFront(T &element);
    void PopFront();

    IntrusiveListIterator<T> begin() const;
    IntrusiveListIterator<T> end() const;

private:

    static T &NodeToObject(IntrusiveListNode<T> *node);
    static IntrusiveListNode<T> *ObjectToNode(T &object);

    static_assert(std::is_base_of_v<IntrusiveListNode<T>, T>);
    static_assert(std::bidirectional_iterator<IntrusiveListIterator<T>>);

    size_t size_;
    std::unique_ptr<IntrusiveListNode<T>> head_;
};

template <typename T>
IntrusiveList<T>::IntrusiveList()
    : size_(0)
{
    head_ = std::make_unique<IntrusiveListNode<T>>();
    head_->prev_ = head_->succ_ = head_.get();
}

template <typename T>
T &IntrusiveListIterator<T>::operator*() const
{
    return *static_cast<T*>(node_);
}

template <typename T>
IntrusiveListIterator<T> &IntrusiveListIterator<T>::operator++()
{
    node_ = node_->succ_;
    return *this;
}

template <typename T>
IntrusiveListIterator<T> IntrusiveListIterator<T>::operator++(int)
{
    auto result = *this;
    ++*this;
    return result;
}

template <typename T>
IntrusiveListIterator<T> &IntrusiveListIterator<T>::operator--()
{
    node_ = node_->prev_;
    return *this;
}

template <typename T>
IntrusiveListIterator<T> IntrusiveListIterator<T>::operator--(int)
{
    auto result = *this;
    --*this;
    return result;
}

template <typename T>
bool IntrusiveListIterator<T>::operator==(const IntrusiveListIterator &other) const
{
    return node_ == other.node_;
}

template <typename T>
IntrusiveList<T>::~IntrusiveList()
{
    Clear();
}

template <typename T>
IntrusiveList<T>::IntrusiveList(IntrusiveList &&other) noexcept
    : IntrusiveList()
{
    this->Swap(other);
}

template <typename T>
IntrusiveList<T> &IntrusiveList<T>::operator=(IntrusiveList &&other) noexcept
{
    this->Swap(other);
    return *this;
}

template <typename T>
void IntrusiveList<T>::Swap(IntrusiveList &other) noexcept
{
    std::swap(size_, other.size_);
    std::swap(head_, other.head_);
}

template <typename T>
size_t IntrusiveList<T>::GetSize() const
{
    return size_;
}

template <typename T>
bool IntrusiveList<T>::IsEmpty() const
{
    return size_ == 0;
}

template <typename T>
void IntrusiveList<T>::Clear()
{
    while(!IsEmpty())
    {
        PopBack();
    }
}

template <typename T>
T &IntrusiveList<T>::GetBack() const
{
    assert(!IsEmpty());
    return IntrusiveList::NodeToObject(head_->prev_);
}

template <typename T>
void IntrusiveList<T>::PushBack(T &element)
{
    auto node = IntrusiveList::ObjectToNode(element);
    assert(!node->prev_ && !node->succ_);

    auto originalBack = head_->prev_;
    originalBack->succ_ = node;
    head_->prev_ = node;
    node->succ_ = head_.get();
    node->prev_ = originalBack;

    ++size_;
}

template <typename T>
void IntrusiveList<T>::PopBack()
{
    assert(!IsEmpty());
    --size_;

    auto back = head_->prev_;
    auto backback = back->prev_;
    backback->succ_ = head_.get();
    head_->prev_ = backback;
    back->prev_ = back->succ_ = nullptr;
}

template <typename T>
T &IntrusiveList<T>::GetFront() const
{
    assert(!IsEmpty());
    return IntrusiveList::NodeToObject(head_->succ_);
}

template <typename T>
void IntrusiveList<T>::PushFront(T &element)
{
    auto node = IntrusiveList::ObjectToNode(element);
    assert(!node->prev_ && !node->succ_);

    auto originalFront = head_->succ_;
    originalFront->prev_ = node;
    head_->succ_ = node;
    node->succ_ = originalFront;
    node->prev_ = head_.get();

    ++size_;
}

template <typename T>
void IntrusiveList<T>::PopFront()
{
    assert(!IsEmpty());
    --size_;

    auto front = head_->succ_;
    auto frontfront = front->succ_;
    frontfront->prev_ = head_.get();
    head_->succ_ = frontfront;
    front->succ_ = front->prev_ = nullptr;
}

template <typename T>
IntrusiveListIterator<T> IntrusiveList<T>::begin() const
{
    IntrusiveListIterator<T> result;
    result.node_ = head_->succ_;
    return result;
}

template <typename T>
IntrusiveListIterator<T> IntrusiveList<T>::end() const
{
    IntrusiveListIterator<T> result;
    result.node_ = head_.get();
    return result;
}

template <typename T>
T &IntrusiveList<T>::NodeToObject(IntrusiveListNode<T> *node)
{
    assert(node);
    return static_cast<T&>(*node);
}

template <typename T>
IntrusiveListNode<T> *IntrusiveList<T>::ObjectToNode(T &object)
{
    return &static_cast<IntrusiveListNode<T> &>(object);
}

RTRC_END
