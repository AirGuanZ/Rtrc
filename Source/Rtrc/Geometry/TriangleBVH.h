#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Vector3.h>

RTRC_BEGIN

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

RTRC_END
