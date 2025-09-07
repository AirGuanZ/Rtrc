#pragma once

#include <stack>

#include <Rtrc/Core/Bit.h>
#include <Rtrc/Core/Memory/Arena.h>
#include <Rtrc/Core/Math/Vector.h>
#include <Rtrc/Core/Unreachable.h>

RTRC_BEGIN

template<typename T = uint32_t>
class QuadTreeAllocator : public Uncopyable
{
public:

    QuadTreeAllocator() = default;
    QuadTreeAllocator(QuadTreeAllocator &&other) noexcept;
    QuadTreeAllocator &operator=(QuadTreeAllocator &&other) noexcept;

    void Swap(QuadTreeAllocator &other) noexcept;

    void Reset(uint32_t log2Size);

    std::optional<Vector2<T>> Allocate(const Vector2<T> &size);
    std::optional<Vector2<T>> Allocate(T log2Size);
    void Free(const Vector2<T> &offset, uint32_t log2Size);

    T GetMaxAvailableSideLen() const;

private:

    struct Node
    {
        Node *parent = nullptr;
        Node *children[4] = { nullptr };

        T maxAvailableSideLen = 0; // for leaf nodes, 0 indicates being occupied
        
        bool IsLeaf() const { return children[0] == nullptr; }
    };

    Node *AllocateNode();

    LinearAllocator nodeAllocator_;
    std::vector<Node *> freeNodes_;
    Node *root_ = nullptr;
    uint32_t size_ = 0;
};

template <typename T>
QuadTreeAllocator<T>::QuadTreeAllocator(QuadTreeAllocator &&other) noexcept
    : QuadTreeAllocator()
{
    this->Swap(other);
}

template <typename T>
QuadTreeAllocator<T> &QuadTreeAllocator<T>::operator=(QuadTreeAllocator &&other) noexcept
{
    this->Swap(other);
    return *this;
}

template <typename T>
void QuadTreeAllocator<T>::Swap(QuadTreeAllocator &other) noexcept
{
    nodeAllocator_.Swap(other.nodeAllocator_);
    freeNodes_.swap(other.freeNodes_);
    std::swap(root_, other.root_);
    std::swap(size_, other.size_);
}

template <typename T>
void QuadTreeAllocator<T>::Reset(uint32_t log2Size)
{
    if(root_)
    {
        std::stack<Node *> nodes;
        nodes.push(root_);

        while(!nodes.empty())
        {
            auto node = nodes.top();
            nodes.pop();
            freeNodes_.push_back(node);

            if(node->children[0])
            {
                for(auto child : node->children)
                {
                    nodes.push(child);
                }
            }
        }
    }

    root_ = AllocateNode();
    root_->maxAvailableSideLen = T(1) << log2Size;
    size_ = T(1) << log2Size;
}

template <typename T>
std::optional<Vector2<T>> QuadTreeAllocator<T>::Allocate(const Vector2<T> &size)
{
    const T log2SideLen = Rtrc::NextPowerOfTwo_Power((std::max)(size.x, size.y));
    return this->Allocate(log2SideLen);
}

template <typename T>
std::optional<Vector2<T>> QuadTreeAllocator<T>::Allocate(T log2Size)
{
    const T sideLen = T(1) << log2Size;
    if(log2Size > root_->maxAvailableSideLen)
    {
        return {};
    }

    Vector2<T> offset = { 0, 0 };
    Node *node = root_;
    uint32_t depth = 0;
    while(true)
    {
        if(node->maxAvailableSideLen == sideLen && node->IsLeaf())
        {
            assert(node->children[0] == nullptr);
            break;
        }

        if(node->children[0] == nullptr)
        {
            for(auto &child : node->children)
            {
                child = AllocateNode();
                child->parent = node;
                child->maxAvailableSideLen = node->maxAvailableSideLen / 2;
            }
            offset = T(2) * offset;
            node = node->children[0];
            ++depth;
            continue;
        }

        assert(node->children[0] && node->children[1] && node->children[2] && node->children[3]);
        bool selected = false;
        for(T i = 0; i < 4; ++i)
        {
            if(auto child = node->children[i]; child->maxAvailableSideLen >= sideLen)
            {
                node = child;
                selected = true;
                offset = T(2) * offset;
                offset.x += i % 2;
                offset.y += i / 2;
                ++depth;
                break;
            }
        }
        assert(selected);
    }

    node->maxAvailableSideLen = 0;
    node = node->parent;
    while(node)
    {
        node->maxAvailableSideLen = (std::max)(
        {
            node->children[0]->maxAvailableSideLen,
            node->children[1]->maxAvailableSideLen,
            node->children[2]->maxAvailableSideLen,
            node->children[3]->maxAvailableSideLen,
        });
        node = node->parent;
    }

    return sideLen * offset;
}

template <typename T>
void QuadTreeAllocator<T>::Free(const Vector2<T> &offset, uint32_t log2Size)
{
    const uint32_t sideLen = T(1) << log2Size;

    // Find the to-be-freed node

    uint32_t currentSideLen = size_;
    Vector2<T> currentOffset = { 0, 0 };
    Node *node = root_;
    while(true)
    {
        if(currentSideLen == sideLen)
        {
            break;
        }
        assert(currentSideLen > 0);

        const uint32_t childSideLen = currentSideLen / 2;
        const uint32_t childSizeRatio = childSideLen / sideLen;
        const Vector2<T> childBaseOffset = T(2) * currentOffset;
        const Vector2<T> offsetAtChildLevel = { offset.x / childSizeRatio, offset.y / childSizeRatio };
        assert(offsetAtChildLevel.x == childBaseOffset.x || offsetAtChildLevel.x == childBaseOffset.x + 1);
        assert(offsetAtChildLevel.y == childBaseOffset.y || offsetAtChildLevel.y == childBaseOffset.y + 1);
        const uint32_t localX = offsetAtChildLevel.x == childBaseOffset.x ? 0 : 1;
        const uint32_t localY = offsetAtChildLevel.y == childBaseOffset.y ? 0 : 1;
        const uint32_t childIndex = 2 * localY + localX;

        node = node->children[childIndex];
        currentOffset = childBaseOffset + Vector2<T>(localX, localY);
        currentSideLen = currentSideLen / 2;
    }
    assert(node->IsLeaf());
    assert(node->maxAvailableSideLen == 0);

    // Mark node as unoccupied

    node->maxAvailableSideLen = currentSideLen;

    // Update ancestors

    node = node->parent;
    assert(!node || currentSideLen > 0);
    currentSideLen *= 2;

    while(true)
    {
        if(!node)
        {
            break;
        }

        T maxChildSideLen = 0;
        bool dontMerge = false;
        assert(!node->IsLeaf());
        for(int i = 0; i < 4; ++i)
        {
            maxChildSideLen = (std::max)(maxChildSideLen, node->children[i]->maxAvailableSideLen);
            dontMerge |= node->children[i]->maxAvailableSideLen != currentSideLen / 2;
        }

        if(dontMerge)
        {
            node->maxAvailableSideLen = maxChildSideLen;
        }
        else
        {
            node->maxAvailableSideLen = currentSideLen;
            for(int i = 0; i < 4; ++i)
            {
                freeNodes_.push_back(node->children[i]);
                node->children[i] = nullptr;
            }
        }

        node = node->parent;
        assert(!node || currentSideLen > 0);
        currentSideLen *= 2;
    }
}

template <typename T>
T QuadTreeAllocator<T>::GetMaxAvailableSideLen() const
{
    return root_->maxAvailableSideLen;
}

template <typename T>
QuadTreeAllocator<T>::Node* QuadTreeAllocator<T>::AllocateNode()
{
    if(!freeNodes_.empty())
    {
        auto node = freeNodes_.back();
        freeNodes_.pop_back();
        *node = Node();
        return node;
    }
    return nodeAllocator_.Create<Node>();
}

RTRC_END
