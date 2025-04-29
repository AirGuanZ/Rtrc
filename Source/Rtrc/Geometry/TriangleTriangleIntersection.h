#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Container/StaticVector.h>
#include <Rtrc/Core/Math/Vector.h>

RTRC_GEO_BEGIN

// See:
//    "Exact predicates, exact constructions and combinatorics for mesh CSG"
//    "Faster Triangle-Triangle Intersection Tests"
template<typename T>
class SymbolicTriangleTriangleIntersection
{
public:

    using Vector = Vector3<T>;

    enum class Type
    {
        None,
        Point,
        Edge,
        Polygon
    };

    enum class Element : uint8_t
    {
        V0,
        V1,
        V2,
        E01,
        E12,
        E20,
        F
    };

    class Point
    {
        uint8_t data_ = 0;

    public:

        Point() = default;
        Point(Element a, Element b): data_(std::to_underlying(a) | (std::to_underlying(b) << 4)) {}
        Element GetElement0() const { return static_cast<Element>(data_ & 0xf); }
        Element GetElement1() const { return static_cast<Element>(data_ >> 4); }
        void SwapOrder() { *this = Point(GetElement1(), GetElement0()); }
        auto operator<=>(const Point &) const = default;
    };

    // Note: both input triangles must be non-degenerate
    static SymbolicTriangleTriangleIntersection Intersect(
        const Vector &a0, const Vector &b0, const Vector &c0,
        const Vector &a1, const Vector &b1, const Vector &c1, bool sortForPolygon);

    void SortPointsForPolygon();

    Type GetType() const;

    Span<Point> GetPoints() const { return points_; }

private:

    static constexpr int MaxIntermediatePointCount = 42;

    static_assert(std::is_floating_point_v<T>);

    using IntermediatePointVector = StaticVector<Point, MaxIntermediatePointCount>;
    using PointVector = StaticVector<Point, 6>;

    static void IntersectEdgeTriangle(
        Element edgePQ, Element vertexP, Element vertexQ,
        const Vector &p, const Vector &q,
        const Vector &a, const Vector &b, const Vector &c,
        IntermediatePointVector &outPoints);

    static void IntersectCoplanarEdgeTriangle(
        Element edgePQ, Element vertexP, Element vertexQ,
        const Vector &p, const Vector &q,
        const Vector &a, const Vector &b, const Vector &c,
        IntermediatePointVector &outPoints);

    static void IntersectCoplanarEdgeEdge(
        Element edgePQ, Element vertexP, Element vertexQ,
        Element edgeAB, Element vertexA, Element vertexB,
        const Vector &p, const Vector &q,
        const Vector &a, const Vector &b,
        const Vector &observer,
        IntermediatePointVector &outPoints);

    static void IntersectColinearEdgeEdge(
        Element edgePQ, Element vertexP, Element vertexQ,
        Element edgeAB, Element vertexA, Element vertexB,
        const Vector &p, const Vector &q,
        const Vector &a, const Vector &b,
        IntermediatePointVector &outPoints);

    static Vector2<T> ProjectTo2D(const Vector &p, int axis);

    static bool IsOnInputEdge(const Point &a, const Point &b);

    static bool IsOnInputEdge(Element a, Element b);

    PointVector points_;
};

RTRC_GEO_END
