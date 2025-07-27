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

    T GetMaxAvailableLog2SideLen() const;

private:

    struct Node
    {
        Node *parent = nullptr;
        Node *children[4] = { nullptr };
        T maxAvailableLog2SideLen = 0; // for leaf nodes, 0 indicates being occupied

        bool IsLeaf() const { return children[0] == nullptr; }
    };

    Node *AllocateNode();

    LinearAllocator nodeAllocator_;
    std::vector<Node *> freeNodes_;
    Node root_;
    uint32_t log2Size_ = 0;
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
}

template <typename T>
void QuadTreeAllocator<T>::Reset(uint32_t log2Size)
{
    std::stack<Node *> nodes;
    if(root_.children[0])
    {
        for(auto child : root_.children)
        {
            nodes.push(child);
        }
    }

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

    root_ = Node();
    root_.maxAvailableLog2SideLen = log2Size;
    log2Size_ = log2Size;
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
    if(log2Size > root_.maxAvailableLog2SideLen)
    {
        return {};
    }

    Vector2<T> offset = { 0, 0 };
    Node *node = &root_;
    uint32_t depth = 0;
    while(true)
    {
        if(node->maxAvailableLog2SideLen == log2Size && node->IsLeaf())
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
                child->maxAvailableLog2SideLen = node->maxAvailableLog2SideLen - 1;
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
            if(auto child = node->children[i]; child->maxAvailableLog2SideLen >= log2Size)
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

    node->maxAvailableLog2SideLen = 0;
    node = node->parent;
    while(node)
    {
        node->maxAvailableLog2SideLen = (std::max)(
        {
            node->children[0]->maxAvailableLog2SideLen,
            node->children[1]->maxAvailableLog2SideLen,
            node->children[2]->maxAvailableLog2SideLen,
            node->children[3]->maxAvailableLog2SideLen,
        });
        node = node->parent;
    }

    const uint32_t nodeSize = 1u << (log2Size_ - depth);
    return nodeSize * offset;
}

template <typename T>
void QuadTreeAllocator<T>::Free(const Vector2<T> &offset, uint32_t log2Size)
{
    // Find the to-be-freed node

    uint32_t currentLog2SideLen = log2Size_;
    Vector2<T> currentOffset = { 0, 0 };
    Node *node = &root_;
    while(true)
    {
        if(currentLog2SideLen == log2Size)
        {
            break;
        }
        assert(currentLog2SideLen > 0);

        const uint32_t childLog2SideLen = currentLog2SideLen - 1;
        const uint32_t childDepthDiff = childLog2SideLen - log2Size;
        const Vector2<T> childBaseOffset = T(2) * currentOffset;
        const Vector2<T> offsetAtChildLevel = { offset.x >> childDepthDiff, offset.y >> childDepthDiff };
        assert(offsetAtChildLevel.x == childBaseOffset.x || offsetAtChildLevel.x == childBaseOffset.x + 1);
        assert(offsetAtChildLevel.y == childBaseOffset.y || offsetAtChildLevel.y == childBaseOffset.y + 1);
        const uint32_t localX = offsetAtChildLevel.x == childBaseOffset.x ? 0 : 1;
        const uint32_t localY = offsetAtChildLevel.y == childBaseOffset.y ? 0 : 1;
        const uint32_t childIndex = 2 * localY + localX;

        node = node->children[childIndex];
        currentOffset = childBaseOffset + Vector2<T>(localX, localY);
        currentLog2SideLen = currentLog2SideLen - 1;
    }
    assert(node->IsLeaf());
    assert(node->maxAvailableLog2SideLen == 0);

    // Mark node as unoccupied

    node->maxAvailableLog2SideLen = currentLog2SideLen;

    // Update ancestors

    node = node->parent;
    assert(!node || currentLog2SideLen > 0);
    --currentLog2SideLen;

    while(true)
    {
        if(!node)
        {
            break;
        }

        T maxChildLog2SideLen = 0;
        bool hasNonLeafChild = false;
        assert(!node->IsLeaf());
        for(int i = 0; i < 4; ++i)
        {
            maxChildLog2SideLen = (std::max)(maxChildLog2SideLen, node->children[i]->maxAvailableLog2SideLen);
            hasNonLeafChild |= !node->children[i]->IsLeaf();
        }

        if(hasNonLeafChild)
        {
            node->maxAvailableLog2SideLen = maxChildLog2SideLen;
        }
        else
        {
            node->maxAvailableLog2SideLen = currentLog2SideLen;
            for(int i = 0; i < 4; ++i)
            {
                freeNodes_.push_back(node->children[i]);
                node->children[i] = nullptr;
            }
        }

        node = node->parent;
        assert(!node || currentLog2SideLen > 0);
        --currentLog2SideLen;
    }
}

template <typename T>
T QuadTreeAllocator<T>::GetMaxAvailableLog2SideLen() const
{
    return root_.maxAvailableLog2SideLen;
}

template <typename T>
typename QuadTreeAllocator<T>::Node* QuadTreeAllocator<T>::AllocateNode()
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
