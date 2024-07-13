#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Intersection.h>
#include <Rtrc/Core/Math/Vector3.h>

RTRC_GEO_BEGIN

class TriangleBVH
{
public:

    static TriangleBVH Build(Span<Vector3f> positions, Span<uint32_t> indices);

    // func(float t, size_t triangleIndex, float2 baryUV)
    template<typename Func>
    void FindClosestIntersection(const Vector3f &o, const Vector3f &d, float maxT, const Func &func) const;

private:

    struct Node
    {
        float boundingBox[6];
        uint32_t childIndex;
        uint32_t childCount; // 0 for interior nodes

        bool IsLeaf() const;
    };
    static_assert(sizeof(Node) == 32);

    struct Triangle
    {
        Vector3f a;
        Vector3f ab;
        Vector3f ac;
    };

    std::vector<Node>     nodes_;
    std::vector<Triangle> triangles_;
    std::vector<uint32_t> originalTriangleIndices_; // map triangle index in 'triangles_' to original primitive index
};

template <typename Func>
void TriangleBVH::FindClosestIntersection(const Vector3f &o, const Vector3f &d, float maxT, const Func &func) const
{
    const Vector3f rcpD = { 1.0f / d.x, 1.0f / d.y, 1.0f / d.z };
    if(!IntersectRayBox(o, rcpD, 0.0f, maxT, nodes_[0].boundingBox))
    {
        return;
    }

    float t = maxT;
    uint32_t primitiveIndex = (std::numeric_limits<uint32_t>::max)();
    Vector2f baryUV;

    constexpr int STACK_SIZE = 128;
    uint32_t stack[STACK_SIZE];
    int top = 0;
    stack[top++] = 0;

    while(top)
    {
        const uint32_t taskNodeIndex = stack[--top];
        const Node &node = nodes_[taskNodeIndex];

        if(node.IsLeaf())
        {
            for(uint32_t i = node.childIndex; i < node.childIndex + node.childCount; ++i)
            {
                const Triangle &triangle = triangles_[i];
                float localT; Vector2f localBaryUV;
                if(IntersectRayTriangle(o, d, 0.0f, t, triangle.a, triangle.ab, triangle.ac, localT, localBaryUV))
                {
                    if(localT < t)
                    {
                        t = localT;
                        baryUV = localBaryUV;
                        primitiveIndex = i;
                    }
                }
            }
        }
        else
        {
            assert(top + 2 < STACK_SIZE);

            if(IntersectRayBox(o, rcpD, 0.0f, t, nodes_[node.childIndex].boundingBox))
            {
                stack[top++] = node.childIndex;
            }

            if(IntersectRayBox(o, rcpD, 0.0f, t, nodes_[node.childIndex + 1].boundingBox))
            {
                stack[top++] = node.childIndex + 1;
            }
        }
    }

    if(primitiveIndex != (std::numeric_limits<uint32_t>::max)())
    {
        const int originalTriangleIndex = originalTriangleIndices_[primitiveIndex];
        func(t, originalTriangleIndex, baryUV);
    }
}

RTRC_GEO_END
