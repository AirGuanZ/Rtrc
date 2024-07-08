#include <random>

#include <Rtrc/Geometry/ConstrainedTriangulation.h>
#include <Rtrc/Geometry/Expansion.h>
#include <Rtrc/Geometry/HalfedgeMesh.h>
#include <Rtrc/Geometry/Predicates.h>

RTRC_GEO_BEGIN

namespace CDTDetail
{

    Expansion3 HomoAdd(const Expansion3 &a, const Expansion3 &b)
    {
        return
        {
            a.x * b.z + b.x * a.z,
            a.y * b.z + b.y * a.z,
            a.z * b.z
        };
    }

    Expansion3 HomoSub(const Expansion3 &a, const Expansion3 &b)
    {
        return
        {
            a.x * b.z - b.x * a.z,
            a.y * b.z - b.y * a.z,
            a.z * b.z
        };
    }

    // See 'Walking in a Triangulation'
    //     separate(T, E):
    //         T: triangle index
    //         E: edge index of triangle T, must be in {0, 1, 2}
    //         Let v be the vertex of T that does not belong to E.
    //         separate(T, E) returns 1 if the target point and v are on opposite sides of E.
    //         If the target point lies exactly on E, it returns 0; otherwise, it returns -1.
    //     getNeighbor(T, E):
    //         Returns the opposite triangle that shares edge E with triangle T.
    //         -1 is returned if no such triangle exists.
    //     generateEdgeGuess():
    //         Returns a value in {0, 1, 2}, giving the initial guess of the edge in each walking step.
    template<typename Separate, typename GetNeighbor, typename GenerateEdgeGuess>
    int LocatePointInTriangulation(
        const Separate          &separate,
        const GetNeighbor       &getNeighbor,
        const GenerateEdgeGuess &generateEdgeGuess,
        int                      initialTriangleGuess,
        int                    (&signs)[3])
    {
        int T = initialTriangleGuess;
        while(true)
        {
            const int E0 = generateEdgeGuess();
            const int S0 = separate(T, E0);
            if(S0 > 0)
            {
                T = getNeighbor(T, E0);
                continue;
            }
            signs[E0] = S0;
            const int E1 = (E0 + 1) % 3;
            const int S1 = separate(T, E1);
            if(S1 > 0)
            {
                T = getNeighbor(T, E1);
                continue;
            }
            signs[E1] = S1;
            const int E2 = (E1 + 1) % 3;
            const int S2 = separate(T, E2);
            if(S2 > 0)
            {
                T = getNeighbor(T, E2);
                continue;
            }
            signs[E2] = S2;
            return T;
        }
    }

    void FlipToDelaunay(HalfedgeMesh &connectivity, Span<Expansion2> points)
    {
        assert(connectivity.IsCompacted());
        std::vector<int> activeEdges = std::views::iota(0, connectivity.E()) | std::ranges::to<std::vector<int>>();
        while(!activeEdges.empty())
        {
            const int e0 = activeEdges.back();
            activeEdges.pop_back();

            const int ha0 = connectivity.EdgeToHalfedge(e0);
            const int hb0 = connectivity.Twin(ha0);
            if(hb0 == HalfedgeMesh::NullID)
            {
                continue;
            }

            const int v0 = connectivity.Head(ha0);
            const int v1 = connectivity.Tail(ha0);
            const int v2 = connectivity.Vert(connectivity.Prev(ha0));
            const int v3 = connectivity.Vert(connectivity.Prev(hb0));
            if(InCircle(points[v0], points[v1], points[v2], points[v3]) <= 0)
            {
                continue;
            }

            const int e1 = connectivity.Edge(connectivity.Succ(ha0));
            const int e2 = connectivity.Edge(connectivity.Prev(ha0));
            const int e3 = connectivity.Edge(connectivity.Succ(hb0));
            const int e4 = connectivity.Edge(connectivity.Prev(hb0));

            connectivity.FlipEdge(e0);

            activeEdges.push_back(e1);
            activeEdges.push_back(e2);
            activeEdges.push_back(e3);
            activeEdges.push_back(e4);
        }
    }

    // Intersect segment ot with triangle oab. All points are represented using homogeneous coordinates.
    // * t is not inside oab or lie on ab (being coincident with a/b is allowed) since we are in a valid triangulation.
    // * oab is in clockwise order.
    // Returns 0 if not intersected;
    //         1 if oa lies on ot;
    //         2 if ob lies on ot;
    //         3 if ot intersects ab.
    int IntersectCornerRay(const Expansion3 &o, const Expansion3 &t, const Expansion3 &a, const Expansion3 &b)
    {
        const int st = Orient2DHomogeneous(a, b, t);
        if(st < 0) // s and o are on the same side of ab
        {
            return 0;
        }
        const int sa = Orient2DHomogeneous(o, t, a);
        const int sb = Orient2DHomogeneous(o, t, b);
        if(sa * sb > 0) // a and b are on the same side of st
        {
            return 0;
        }
        return sa == 0 ? 1 : (sb == 0 ? 2 : 3);
    }

    // Compute intersection between homogeneous segments ab and cd.
    // The intersection must exist uniquely, and is not at a, b, c, or d.
    Expansion3 ComputeIntersectionHomogeneous(
        const Expansion3 &a, const Expansion3 &b, const Expansion3 &c, const Expansion3 &d)
    {
        const Expansion3 A = HomoSub(b, a);
        const Expansion3 B = HomoSub(c, d);
        const Expansion3 C = HomoSub(c, a);
        const Expansion2 det = Expansion2(A.x * B.y - B.x * A.y, A.z * B.z);
        const Expansion2 det1 = Expansion2(C.x * B.y - B.x * C.y, C.z * B.z);
        const Expansion2 t = Expansion2(det1.x * det.y, det1.y * det.x);
        return HomoAdd(a, Expansion3(A.x * t.x, A.y * t.x, A.z * t.y));
    }

    // Intersect segment ot with triangle abc.
    // ot passes through edge ab and this function evaluates the other intersection.
    // * abc is in clockwise order.
    // * t doesn't lie on ac or bc (being coincident with c is allowed).
    // Returns 0 if ot passes through c;
    //         1 if ot intersects ac;
    //         2 if ot intersects bc.
    int IntersectEdgeRay(
        const Expansion3 &o, const Expansion3 &t, const Expansion3 &a, const Expansion3 &b, const Expansion3 &c)
    {
        const int sc = Orient2DHomogeneous(o, t, c);
        if(sc == 0) // o, t, c are colinear
        {
            return 0;
        }
        const int sa = Orient2DHomogeneous(o, t, a);
        if(sa == sc) // a and c sit on the same side of ot, so ot must intersect bc
        {
            return 2;
        }
        assert(Orient2DHomogeneous(o, t, b) == sc);
        return 1;
    }

    // Test if ab intersects cd. Returns false when three points are colinear.
    bool TestIntersectionNoColinear(const Expansion3 &a, const Expansion3 &b, const Expansion3 &c, const Expansion3 &d)
    {
        const int sa = Orient2DHomogeneous(c, d, a);
        const int sb = Orient2DHomogeneous(c, d, b);
        if(sa * sb >= 0)
        {
            return false;
        }
        const int sc = Orient2DHomogeneous(a, b, c);
        const int sd = Orient2DHomogeneous(a, b, d);
        if(sc * sd >= 0)
        {
            return false;
        }
        return true;
    }

    // Test whether quad acbd is convex. abc and adb must be in clockwise order.
    bool IsConvexQuad(const Expansion3 &a, const Expansion3 &b, const Expansion3 &c, const Expansion3 &d)
    {
        // The quad is convex iff flipping ab gives two correctly-oriented triangles.
        return Orient2DHomogeneous(a, d, c) < 0 && Orient2DHomogeneous(c, d, b) < 0;
    }

} // namespace CDTDetail

std::vector<Vector3i> ConstrainedTriangulation2D(
    Span<Expansion2>         points,
    Span<Vector2i>           constraints,
    std::vector<Expansion3> &newIntersections)
{
    using namespace CDTDetail;

    assert(points.size() >= 3);

    // Build super triangle

    Vector2d lower(DBL_MAX), upper(DBL_MIN);
    for(const Expansion2 &point : points)
    {
        lower.x = (std::min)(lower.x, point.x.ToDoubleRoundDown());
        lower.y = (std::min)(lower.y, point.y.ToDoubleRoundDown());
        upper.x = (std::max)(upper.x, point.x.ToDoubleRoundUp());
        upper.y = (std::max)(upper.y, point.y.ToDoubleRoundUp());
    }
    const double extent = MaxReduce(upper - lower);
    const Vector2d superTriangleA = lower - Vector2d(extent);
    const Vector2d superTriangleB = superTriangleA + Vector2d(0, 8 * extent);
    const Vector2d superTriangleC = superTriangleA + Vector2d(8 * extent, 0);

    // Initialize connectivity

    std::vector<Expansion3> vertices =
    {
        // In homogeneous coordinate
        ToExpansion(Vector3d(superTriangleA, 1.0)),
        ToExpansion(Vector3d(superTriangleB, 1.0)),
        ToExpansion(Vector3d(superTriangleC, 1.0))
    };
    std::vector<int> meshVertexToPointIndex = { -1, -1, -1 };
    HalfedgeMesh connectivity = HalfedgeMesh::Build({ 0, 1, 2 });

    // Insert points

    std::minstd_rand edgeRNG(42);
    std::uniform_int_distribution<int> edgeDistribution(0, 2);

    for(int pointIndex = 0; pointIndex < static_cast<int>(points.size()); ++pointIndex)
    {
        const Expansion2 &point = points[pointIndex];
        int signs[3];
        const int triangleIndex = LocatePointInTriangulation(
            [&](int T, int E)
            {
                const int h = 3 * T + E;
                const int head = connectivity.Head(h);
                const int tail = connectivity.Tail(h);
                const int sign = Orient2D(&vertices[head].x, &vertices[tail].x, &point.x);
                return sign;
            },
            [&](int T, int E)
            {
                const int h = 3 * T + E;
                const int twin = connectivity.Twin(h);
                return twin >= 0 ? twin / 3 : -1;
            },
            [&]
            {
                return edgeDistribution(edgeRNG);
            },
            0, signs);

        // zeroSignCount == 3 means the point lies on three edges simulatiously;
        // zeroSignCount == 2 means the point lies exactly on one exiting vertex.
        // Both cases indicates this is a degenerate triangle.
        const int zeroSignCount = (signs[0] == 0) + (signs[1] == 0) + (signs[2] == 0);
        assert(zeroSignCount < 2);

        if(zeroSignCount == 0) // the point lies inside a triangle.
        {
            connectivity.SplitFace(triangleIndex);
        }
        else if(zeroSignCount == 1) // the point lies on an edge. split it.
        {
            const int h = 3 * triangleIndex + (!signs[0]? 0 : (!signs[1] ? 1 : 2));
            connectivity.SplitEdge(h);
        }

        vertices.emplace_back(point, 1.0);
        meshVertexToPointIndex.push_back(pointIndex);
    }

    // Initialize delaunay conditions

    //if(delaunay)
    //{
    //    FlipToDelaunay(connectivity, points);
    //}

    // Insert edges

    std::vector isEdgeFixed(connectivity.E(), false);

    for(int constraintIndex = 0; constraintIndex < static_cast<int>(constraints.size()); ++constraintIndex)
    {
        const Vector2i &constraint = constraints[constraintIndex];

        int vo = constraint.x + 3;
        const int vt = constraint.y + 3;
        assert(meshVertexToPointIndex[vo] == constraint.x);
        assert(meshVertexToPointIndex[vt] == constraint.y);

        // Used when last intersection is on an edge.
        // In the next iteration we enter triangle `startHalfedge/3` at `startHalfedge`.
        int startHalfedge = -1;

        // Used when last intersection is on an vertex
        int startVertex = vo;

        // Non-fixed edges intersected with ot
        std::deque<int> intersectedEdges;

        // Create constraint edges from vStart to vEnd.
        // `intersectedEdges` stores all edges intersected along the path. None of them is constrained.
        auto CommitIntersectedEdges = [&](int vStart, int vEnd)
        {
            if(vStart == vEnd)
            {
                return;
            }

            std::vector<int> newEdges;
            while(!intersectedEdges.empty())
            {
                const int e = intersectedEdges.front();
                intersectedEdges.pop_front();

                const int h0 = connectivity.EdgeToHalfedge(e);
                const int h1 = connectivity.Twin(h0);
                assert(h1 >= 0); // e intersects (vStart, vEnd), so it must not be a boundary edge

                const int va = connectivity.Head(h0);
                const int vb = connectivity.Tail(h0);
                const int vc = connectivity.Vert(connectivity.Prev(h0));
                const int vd = connectivity.Vert(connectivity.Prev(h1));
                if(IsConvexQuad(vertices[va], vertices[vb], vertices[vc], vertices[vd]))
                {
                    connectivity.FlipEdge(e);
                }

                const int newH0 = connectivity.EdgeToHalfedge(e);
                const int newVa = connectivity.Head(newH0);
                const int newVb = connectivity.Tail(newH0);
                if(TestIntersectionNoColinear(vertices[newVa], vertices[newVb], vertices[vStart], vertices[vEnd]))
                {
                    intersectedEdges.push_back(e);
                }
                else
                {
                    newEdges.push_back(e);
                }
            }
        };

        while(true)
        {
            if(startVertex >= 0)
            {
                if(startVertex == vt) 
                {
                    CommitIntersectedEdges(vo, vt);
                    break;
                }

                connectivity.ForEachOutgoingHalfedge(
                    startVertex, [&](int h)
                    {
                        const int va = connectivity.Vert(connectivity.Succ(h));
                        const int vb = connectivity.Vert(connectivity.Prev(h));
                        const int intersectionType = IntersectCornerRay(
                            vertices[startVertex], vertices[vt], vertices[va], vertices[vb]);
                        if(intersectionType == 0)
                        {
                            return true;
                        }

                        if(intersectionType == 1)
                        {
                            const int e = connectivity.Edge(h);
                            isEdgeFixed[e] = true;
                            if(vo != startVertex)
                            {
                                CommitIntersectedEdges(vo, startVertex);
                            }
                            vo = va;
                            startVertex = vo;
                        }
                        else if(intersectionType == 2)
                        {
                            const int e = connectivity.Edge(connectivity.Prev(h));
                            isEdgeFixed[e] = true;
                            if(vo != startVertex)
                            {
                                CommitIntersectedEdges(vo, startVertex);
                            }
                            vo = vb;
                            startVertex = vo;
                        }
                        else
                        {
                            assert(intersectionType == 3);
                            const int e = connectivity.Edge(connectivity.Succ(h));
                            if(isEdgeFixed[e])
                            {
                                const Expansion3 inct = ComputeIntersectionHomogeneous(
                                    vertices[startVertex], vertices[vt], vertices[va], vertices[vb]);

                                const int vInct = connectivity.V();
                                vertices.push_back(inct);
                                connectivity.SplitEdge(e);

                                assert(static_cast<int>(meshVertexToPointIndex.size()) == vInct);
                                meshVertexToPointIndex.push_back(static_cast<int>(points.size() + newIntersections.size()));
                                newIntersections.push_back(inct);

                                CommitIntersectedEdges(vo, vInct);

                                vo = vInct;
                                startVertex = vo;
                            }
                            else
                            {
                                intersectedEdges.push_back(e);

                                startVertex = -1;
                                startHalfedge = connectivity.Twin(connectivity.Succ(h));
                                assert(startHalfedge >= 0);
                            }
                        }

                        return false;
                    });
            }
            else
            {
                assert(startHalfedge >= 0);
                
                const int va = connectivity.Vert(startHalfedge);
                const int vb = connectivity.Vert(connectivity.Succ(startHalfedge));
                const int vc = connectivity.Vert(connectivity.Prev(startHalfedge));

                const int intersectionType = IntersectEdgeRay(
                    vertices[vo], vertices[vt], vertices[va], vertices[vb], vertices[vc]);

                if(intersectionType == 0)
                {
                    CommitIntersectedEdges(vo, vc);
                    startHalfedge = -1;
                    vo = vc;
                    startVertex = vo;
                }
                else if(intersectionType == 1)
                {
                    const int hac = connectivity.Prev(startHalfedge);
                    const int eac = connectivity.Edge(hac);
                    intersectedEdges.push_back(eac);

                    startHalfedge = connectivity.Twin(hac);
                    assert(startHalfedge >= 0);
                }
                else
                {
                    assert(intersectionType == 2);

                    const int hbc = connectivity.Succ(startHalfedge);
                    const int ebc = connectivity.Edge(hbc);
                    intersectedEdges.push_back(ebc);

                    startHalfedge = connectivity.Twin(hbc);
                    assert(startHalfedge >= 0);
                }
            }
        }
    }

    std::vector<Vector3i> result;
    for(int f = 0; f < connectivity.F(); ++f)
    {
        const int v0 = connectivity.Vert(3 * f + 0);
        const int v1 = connectivity.Vert(3 * f + 1);
        const int v2 = connectivity.Vert(3 * f + 2);
        if(v0 < 3 || v1 < 3 || v2 < 3)
        {
            continue;
        }
        result.emplace_back(
            meshVertexToPointIndex[v0],
            meshVertexToPointIndex[v1],
            meshVertexToPointIndex[v2]);
    }

    return result;
}

RTRC_GEO_END
