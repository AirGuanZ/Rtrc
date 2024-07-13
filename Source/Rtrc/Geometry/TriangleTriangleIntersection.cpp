#include <Rtrc/Core/Unreachable.h>
#include <Rtrc/Geometry/Exact/Expansion.h>
#include <Rtrc/Geometry/Exact/Intersection.h>
#include <Rtrc/Geometry/Exact/Predicates.h>
#include <Rtrc/Geometry/TriangleTriangleIntersection.h>

RTRC_GEO_BEGIN

template <typename T>
SymbolicTriangleTriangleIntersection<T> SymbolicTriangleTriangleIntersection<T>::Intersect(
    const Vector &a0, const Vector &b0, const Vector &c0,
    const Vector &a1, const Vector &b1, const Vector &c1,
    bool sortForPolygon)
{
    assert(a0 != b0 && a0 != c0 && b0 != c0);
    assert(a1 != b1 && a1 != c1 && b1 != c1);

    IntermediatePointVector points;
    IntersectEdgeTriangle(Element::E01, Element::V0, Element::V1, a0, b0, a1, b1, c1, points);
    IntersectEdgeTriangle(Element::E12, Element::V1, Element::V2, b0, c0, a1, b1, c1, points);
    IntersectEdgeTriangle(Element::E20, Element::V2, Element::V0, c0, a0, a1, b1, c1, points);

    const uint32_t midPoint = points.size();
    IntersectEdgeTriangle(Element::E01, Element::V0, Element::V1, a1, b1, a0, b0, c0, points);
    IntersectEdgeTriangle(Element::E12, Element::V1, Element::V2, b1, c1, a0, b0, c0, points);
    IntersectEdgeTriangle(Element::E20, Element::V2, Element::V0, c1, a1, a0, b0, c0, points);
    for(uint32_t i = midPoint; i < points.size(); ++i)
    {
        points[i].SwapOrder();
    }

    if(!points.empty())
    {
        std::ranges::sort(points);
        const uint32_t newSize = std::unique(points.begin(), points.end()) - points.begin();
        points.Resize(newSize);
    }

    SymbolicTriangleTriangleIntersection ret;

    assert(points.size() <= 6);
    ret.points_.Resize(points.size());
    std::memcpy(ret.points_.GetData(), points.GetData(), ret.points_.size() * sizeof(Point));

    if(sortForPolygon)
    {
        ret.SortPointsForPolygon();
    }
    return ret;
}

template <typename T>
typename SymbolicTriangleTriangleIntersection<T>::Type SymbolicTriangleTriangleIntersection<T>::GetType() const
{
    switch(points_.size())
    {
    case 0:  return Type::None;
    case 1:  return Type::Point;
    case 2:  return Type::Edge;
    default: return Type::Polygon;
    }
}

template <typename T>
void SymbolicTriangleTriangleIntersection<T>::SortPointsForPolygon()
{
    if(GetType() != Type::Polygon)
    {
        return;
    }

    auto FindNeighbor = [&](int p, int ignore)
    {
        for(int i = 0; i < static_cast<int>(points_.GetSize()); ++i)
        {
            if(i != p && i != ignore && IsOnInputEdge(points_[p], points_[i]))
            {
                return i;
            }
        }
        Unreachable();
    };

    PointVector newPoints;
    int ignore = 0, last = FindNeighbor(0, -1);
    newPoints.push_back(points_[0]);
    newPoints.push_back(points_[last]);
    while(newPoints.size() < points_.size())
    {
        const int neighbor = FindNeighbor(last, ignore);
        newPoints.push_back(points_[neighbor]);
        ignore = last;
        last = neighbor;
    }

    points_ = newPoints;
}

template <typename T>
void SymbolicTriangleTriangleIntersection<T>::IntersectEdgeTriangle(
    Element edgePQ, Element vertexP, Element vertexQ,
    const Vector &p, const Vector &q,
    const Vector &a, const Vector &b, const Vector &c,
    IntermediatePointVector &outPoints)
{
    const int s0 = Geo::Orient3D<T>(a, b, c, p);
    const int s1 = Geo::Orient3D<T>(a, b, c, q);
    if(s0 * s1 > 0) // p and q are on the same side of abc, no intersection
    {
        return;
    }
    if(s0 == 0 && s1 == 0) // pq and abc are coplanar. Use 2d code path.
    {
        return IntersectCoplanarEdgeTriangle(edgePQ, vertexP, vertexQ, p, q, a, b, c, outPoints);
    }

    const int o0 = Geo::Orient3D(p, q, a, b);
    const int o1 = Geo::Orient3D(p, q, b, c);
    const int o2 = Geo::Orient3D(p, q, c, a);
    if(o0 * o1 < 0 || o1 * o2 < 0 || o2 * o0 < 0) // Intersection of pq and plane of abc is out of abc
    {
        return;
    }

    const Element firstElement = s0 == 0 ? vertexP : s1 == 0 ? vertexQ : edgePQ;

    constexpr Element SecondElementLUT[] =
    {
        Element::F,   // 000, any 4 points in pqabc are not coplanar
        Element::E01, // 001, pqab are coplanar
        Element::E12, // 010, pqbc
        Element::V1,  // 011, pqab, pqbc
        Element::E20, // 100, pqca
        Element::V0,  // 101, pqab, pqca
        Element::V2,  // 110, pqbc, pqca
        Element::F,   // 111, degenerate case
    };
    assert(o0 || o1 || o2); // This evaluates to false only in degenerate case
    const int index = (o2 == 0) * 4 + (o1 == 0) * 2 + (o0 == 0);
    const Element secondElement = SecondElementLUT[index];

    outPoints.push_back({ firstElement, secondElement });
}

template <typename T>
void SymbolicTriangleTriangleIntersection<T>::IntersectCoplanarEdgeTriangle(
    Element edgePQ, Element vertexP, Element vertexQ,
    const Vector &p, const Vector &q,
    const Vector &a, const Vector &b, const Vector &c,
    IntermediatePointVector &outPoints)
{
    Vector observer;
    {
        // Construct an observer point which is not coplanar with other points
        const int offsetAxis = FindMaximalNormalAxis(a, b, c);
        observer = p;
        observer[offsetAxis] = (std::max)(T(2.0) * p[offsetAxis] + T(1.0), T(10));
    }
    auto ProjectedOrient2D = [&](const Vector &i, const Vector &j, const Vector &k)
    {
        return Geo::Orient3D(i, j, k, observer);
    };

    const int p0 = ProjectedOrient2D(a, b, p);
    const int p1 = ProjectedOrient2D(b, c, p);
    const int p2 = ProjectedOrient2D(c, a, p);
    const int q0 = ProjectedOrient2D(a, b, q);
    const int q1 = ProjectedOrient2D(b, c, q);
    const int q2 = ProjectedOrient2D(c, a, q);

    int fallWithinCount = 0;
    if(p0 * p1 > 0 && p1 * p2 > 0) // abp, bcp, cap has the same winding order, so p must fall within abc
    {
        assert(p2 * p0 > 0);
        outPoints.push_back({ vertexP, Element::F });
        ++fallWithinCount;
    }
    if(q0 * q1 > 0 && q1 * q2 > 0)
    {
        assert(q2 * q0 > 0);
        outPoints.push_back({ vertexQ, Element::F });
        ++fallWithinCount;
    }
    if(fallWithinCount >= 2)
    {
        return;
    }

    IntersectCoplanarEdgeEdge(
        edgePQ, vertexP, vertexQ,
        Element::E01, Element::V0, Element::V1,
        p, q, a, b, observer, outPoints);

    IntersectCoplanarEdgeEdge(
        edgePQ, vertexP, vertexQ,
        Element::E12, Element::V1, Element::V2,
        p, q, b, c, observer, outPoints);

    IntersectCoplanarEdgeEdge(
        edgePQ, vertexP, vertexQ,
        Element::E20, Element::V2, Element::V0,
        p, q, c, a, observer, outPoints);
}

template <typename T>
void SymbolicTriangleTriangleIntersection<T>::IntersectCoplanarEdgeEdge(
    Element edgePQ, Element vertexP, Element vertexQ,
    Element edgeAB, Element vertexA, Element vertexB,
    const Vector &p, const Vector &q,
    const Vector &a, const Vector &b,
    const Vector &observer,
    IntermediatePointVector &outPoints)
{
    auto ProjectedOrient2D = [&](const Vector &i, const Vector &j, const Vector &k)
    {
        return Geo::Orient3D(i, j, k, observer);
    };

    const int sp = ProjectedOrient2D(a, b, p);
    const int sq = ProjectedOrient2D(a, b, q);
    if(sp * sq > 0)
    {
        return;
    }

    if(sp == 0 && sq == 0)
    {
        IntersectColinearEdgeEdge(edgePQ, vertexP, vertexQ, edgeAB, vertexA, vertexB, p, q, a, b, outPoints);
        return;
    }

    const int sa = ProjectedOrient2D(p, q, a);
    const int sb = ProjectedOrient2D(p, q, b);
    assert(sa || sb);
    if(sa * sb > 0)
    {
        return;
    }

    const Element firstElement = sp == 0 ? vertexP : sq == 0 ? vertexQ : edgePQ;
    const Element secondElement = sa == 0 ? vertexA : sb == 0 ? vertexB : edgeAB;
    outPoints.push_back({ firstElement, secondElement });
}

template <typename T>
void SymbolicTriangleTriangleIntersection<T>::IntersectColinearEdgeEdge(
    Element edgePQ, Element vertexP, Element vertexQ,
    Element edgeAB, Element vertexA, Element vertexB,
    const Vector &p, const Vector &q,
    const Vector &a, const Vector &b,
    IntermediatePointVector &outPoints)
{
    auto DotSign = [](const Vector &i, const Vector &j, const Vector &k)
    {
        using E = SExpansion;
        const E ix = E(i.x);
        const E iy = E(i.y);
        const E iz = E(i.z);
        const E ux = E(j.x) - ix;
        const E uy = E(j.y) - iy;
        const E uz = E(j.z) - iz;
        const E vx = E(k.x) - ix;
        const E vy = E(k.y) - iy;
        const E vz = E(k.z) - iz;
        const E dot = ux * vx + uy * vy + uz * vz;
        return dot.GetSign();
    };

    if(p == a)
    {
        outPoints.push_back({ vertexP, vertexA });
    }
    else if(p == b)
    {
        outPoints.push_back({ vertexP, vertexB });
    }
    if(q == a)
    {
        outPoints.push_back({ vertexQ, vertexA });
    }
    else if(q == b)
    {
        outPoints.push_back({ vertexQ, vertexB });
    }

    if(const int s0 = DotSign(p, a, b); s0 < 0) // dot(pa, pb) < 0, so p is between a and b
    {
        outPoints.push_back({ vertexP, edgeAB });
    }
    if(const int s1 = DotSign(q, a, b); s1 < 0)
    {
        outPoints.push_back({ vertexQ, edgeAB });
    }
    if(const int s2 = DotSign(a, p, q); s2 < 0)
    {
        outPoints.push_back({ edgePQ, vertexA });
    }
    if(const int s3 = DotSign(b, p, q); s3 < 0)
    {
        outPoints.push_back({ edgePQ, vertexB });
    }
}

template <typename T>
Vector2<T> SymbolicTriangleTriangleIntersection<T>::ProjectTo2D(const Vector &p, int axis)
{
    return { p[(axis + 1) % 3], p[(axis + 2) % 3] };
}

template <typename T>
bool SymbolicTriangleTriangleIntersection<T>::IsOnInputEdge(const Point &a, const Point &b)
{
    return IsOnInputEdge(a.GetElement0(), b.GetElement0()) ||
           IsOnInputEdge(a.GetElement1(), b.GetElement1());
}

template <typename T>
bool SymbolicTriangleTriangleIntersection<T>::IsOnInputEdge(Element a, Element b)
{
    constexpr bool V0 = false;
    constexpr bool V1 = true;
    static constexpr bool LUT[7][7] =
    {
        { V0, V1, V1, V1, V0, V1, V0 },
        { V1, V0, V1, V1, V1, V0, V0 },
        { V1, V1, V0, V0, V1, V1, V0 },
        { V1, V1, V0, V1, V0, V0, V0 },
        { V0, V1, V1, V0, V1, V0, V0 },
        { V1, V0, V1, V0, V0, V1, V0 },
        { V0, V0, V0, V0, V0, V0, V0 }
    };
    return LUT[std::to_underlying(a)][std::to_underlying(b)];
}

template class SymbolicTriangleTriangleIntersection<float>;
template class SymbolicTriangleTriangleIntersection<double>;

RTRC_GEO_END
