#pragma once

#include <cassert>
#include <ranges>
#include <stack>

#include <Rtrc/Core/Memory/Arena.h>

RTRC_BEGIN

class RGLabelStack
{
public:

    struct Node
    {
        std::string name;
        const Node *parent = nullptr;
        uint32_t depth = 0;
    };

    void Push(std::string name);

    void Pop();

    const Node *GetCurrentNode() const;

    template<typename PushOperation, typename PopOperation>
    static void Transfer(const Node *from, const Node *to, const PushOperation &push, const PopOperation &pop);

private:

    LinearAllocator nodeAlloc_;
    std::stack<const Node *> nodeStack_;
};

inline void RGLabelStack::Push(std::string name)
{
    auto node = nodeAlloc_.Create<Node>();
    node->name = std::move(name);
    node->parent = GetCurrentNode();
    node->depth = static_cast<uint32_t>(nodeStack_.size());
    nodeStack_.push(node);
}

inline void RGLabelStack::Pop()
{
    assert(!nodeStack_.empty());
    nodeStack_.pop();
}

inline const RGLabelStack::Node *RGLabelStack::GetCurrentNode() const
{
    return nodeStack_.empty() ? nullptr : nodeStack_.top();
}

template<typename PushOperation, typename PopOperation>
void RGLabelStack::Transfer(const Node *from, const Node *to, const PushOperation &push, const PopOperation &pop)
{
    if(!from && !to)
    {
        return;
    }
    if(!from)
    {
        assert(to);
        Transfer(nullptr, to->parent, push, pop);
        push(to);
        return;
    }
    if(!to)
    {
        assert(from);
        pop(from);
        Transfer(from->parent, nullptr, push, pop);
        return;
    }

    const int minDepth = std::min(from->depth, to->depth);
    std::vector<const Node *> fromPath, toPath;
    while(from && from->depth != minDepth)
    {
        fromPath.push_back(from);
        from = from->parent;
    }
    while(to && to->depth != minDepth)
    {
        toPath.push_back(to);
        to = to->parent;
    }
    while(from != to)
    {
        assert(from && to);
        fromPath.push_back(from);
        toPath.push_back(to);
        from = from->parent;
        to = to->parent;
    }

    for(auto node : fromPath)
    {
        pop(node);
    }
    for(auto node : std::views::reverse(toPath))
    {
        push(node);
    }
}

RTRC_END
