#pragma once

#include <Rtrc/Core/Math/AABB.h>
#include <Rtrc/Core/Math/Intersection.h>
#include <Rtrc/Geometry/Utility.h>

RTRC_GEO_BEGIN

template<typename T>
class BVH
{
public:

    static BVH Build(Span<AABB3<T>> primitiveBounds);

    bool IsEmpty() const;

    // bool intersectBox(const AABB3<T> &boundingBox)
    // void processPrimitive(uint32_t primitiveIndex)
    // bool processPrimitive(uint32_t primitiveIndex), returns false to stop the traversal.
    template<typename IntersectBoxFunc, typename ProcessPrimitiveFunc>
    void TraversalPrimitives(const IntersectBoxFunc &intersectBox, const ProcessPrimitiveFunc &processPrimitive) const;

private:

    struct Node
    {
        AABB3<T> boundingBox;
        uint32_t childIndex;
        uint32_t childCount; // 0 for interior nodes

        bool IsLeaf() const;
    };

    std::vector<Node>     nodes_;
    std::vector<uint32_t> primitiveIndices_;
};

template<typename T>
class TriangleBVH : BVH<T>
{
public:

    struct RayIntersectionResult
    {
        T t;
        uint32_t triangleIndex;
        Vector2<T> barycentricCoordinate;
    };

    using BVH<T>::IsEmpty;
    using BVH<T>::TraversalPrimitives;

    static TriangleBVH Build(const IndexedPositions<T> &positions);

    bool FindClosestRayIntersection(
        const Vector3<T>      &o,
        const Vector3<T>      &d,
        T                      minT,
        T                      maxT,
        RayIntersectionResult &result) const;

private:

    struct Triangle
    {
        Vector3<T> a;
        Vector3<T> ab;
        Vector3<T> ac;
    };

    std::vector<Triangle> triangles_;
};

template <typename T>
bool BVH<T>::IsEmpty() const
{
    return primitiveIndices_.empty();
}

template <typename T>
template <typename IntersectBoxFunc, typename ProcessPrimitiveFunc>
void BVH<T>::TraversalPrimitives(
    const IntersectBoxFunc &intersectBox, const ProcessPrimitiveFunc &processPrimitive) const
{
    if(IsEmpty())
    {
        return;
    }

    constexpr int STACK_SIZE = 100;
    uint32_t stack[STACK_SIZE];
    int top = 0;
    stack[top++] = 0;

    while(top)
    {
        const uint32_t nodeIndex = stack[--top];
        const Node &node = nodes_[nodeIndex];

        if(node.IsLeaf())
        {
            const uint32_t childEnd = node.childIndex + node.childCount;
            for(uint32_t i = node.childIndex; i < childEnd; ++i)
            {
                processPrimitive(primitiveIndices_[i]);
            }
            continue;
        }

        assert(top + 2 < STACK_SIZE && "BVH stack overflows!");
        if(intersectBox(nodes_[node.childIndex].boundingBox))
        {
            stack[top++] = node.childIndex;
        }
        if(intersectBox(nodes_[node.childIndex + 1].boundingBox))
        {
            stack[top++] = node.childIndex + 1;
        }
    }
}

template <typename T>
bool BVH<T>::Node::IsLeaf() const
{
    return childCount != 0;
}

template <typename T>
TriangleBVH<T> TriangleBVH<T>::Build(const IndexedPositions<T> &positions)
{
    const uint32_t triangleCount = positions.GetSize() / 3;
    std::vector<AABB3<T>> boundingBoxes(triangleCount);
    for(uint32_t f = 0; f < triangleCount; ++f)
    {
        AABB3<T> bbox;
        bbox |= positions[3 * f + 0];
        bbox |= positions[3 * f + 1];
        bbox |= positions[3 * f + 2];
        boundingBoxes[f] = bbox;
    }

    TriangleBVH ret;
    static_cast<BVH<T> &>(ret) = BVH<T>::Build(boundingBoxes);

    ret.triangles_.resize(triangleCount);
    for(uint32_t f = 0; f < triangleCount; ++f)
    {
        const Vector3<T> &a = positions[3 * f + 0];
        const Vector3<T> &b = positions[3 * f + 1];
        const Vector3<T> &c = positions[3 * f + 2];
        ret.triangles_[f] = { a, b - a, c - a };
    }

    return ret;
}

template <typename T>
bool TriangleBVH<T>::FindClosestRayIntersection(
    const Vector3<T>      &o,
    const Vector3<T>      &d,
    T                      minT,
    T                      maxT,
    RayIntersectionResult &result) const
{
    const Vector3<T> rcpD = { 1 / d.x, 1 / d.y, 1 / d.z };

    result.t                     = maxT;
    result.triangleIndex         = UINT32_MAX;
    result.barycentricCoordinate = { 0, 0 };

    BVH<T>::TraversalPrimitives(
        [&](const AABB3<T> &bbox)
        {
            return Rtrc::IntersectRayBox(o, rcpD, minT, result.t, &bbox.lower.x);
        },
        [&](uint32_t triangleIndex)
        {
            float tempT; Vector2<T> tempUV;
            const Triangle &triangle = triangles_[triangleIndex];
            if(IntersectRayTriangle(o, d, minT, result.t, triangle.a, triangle.ab, triangle.ac, tempT, tempUV))
            {
                result.t                     = tempT;
                result.triangleIndex         = triangleIndex;
                result.barycentricCoordinate = tempUV;
            }
        });

    return result.triangleIndex != UINT32_MAX;
}

RTRC_GEO_END