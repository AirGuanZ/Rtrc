#include <map>
#include <set>

#include <Rtrc/Core/Math/AABB.h>
#include <Rtrc/Core/Math/Exact/Intersection.h>
#include <Rtrc/Core/Math/Exact/Predicates.h>
#include <Rtrc/Core/Parallel.h>
#include <Rtrc/Core/Unreachable.h>
#include <Rtrc/Geometry/BVH.h>
#include <Rtrc/Geometry/ConstrainedTriangulation.h>
#include <Rtrc/Geometry/MeshCorefinement.h>
#include <Rtrc/Geometry/TriangleTriangleIntersection.h>
#include <Rtrc/Geometry/Utility.h>

//#define RTRC_MESH_COREFINEMENT_PARALLEL_FOR Rtrc::ParallelForDebug
#define RTRC_MESH_COREFINEMENT_PARALLEL_FOR Rtrc::ParallelFor

RTRC_GEO_BEGIN

namespace CorefineDetail
{

    using SI = SymbolicTriangleTriangleIntersection<double>;

    // Returns true if abc and abd only intersect at edge ab.
    // This method is intended for coarse filtering only; false negatives are permissible.
    bool CantHaveInterestingIntersections(const Vector3d &a, const Vector3d &b, const Vector3d &c, const Vector3d &d)
    {
        // d is not on plane abc
        if(Orient3DApprox(a, b, c, d))
        {
            return true;
        }

        // c and d are on different sides of ab
        for(int i = 0; i < 3; ++i)
        {
            const int axis0 = i;
            const int axis1 = (i + 1) % 3;
            const Vector2d a2d = { a[axis0], a[axis1] };
            const Vector2d b2d = { b[axis0], b[axis1] };
            const Vector2d c2d = { c[axis0], c[axis1] };
            const Vector2d d2d = { d[axis0], d[axis1] };
            const int o0 = Orient2DApprox(a2d, b2d, c2d);
            const int o1 = Orient2DApprox(a2d, b2d, d2d);
            if(o0 * o1 < 0)
            {
                return true;
            }
        }

        return false;
    }

    // result[triangleA] is { triangleB | triangleA < triangleB and triangleA is adjacent to triangleB }
    std::vector<std::vector<int>> DetectAdjacentTriangles(const IndexedPositions<double> &input)
    {
        struct TriangleRecord
        {
            Vector3d oppositeVertex;
            int triangleIndex;
        };
        std::map<std::pair<Vector3d, Vector3d>, std::vector<TriangleRecord>> edgeToTriangles;

        auto HandleEdge = [&](const Vector3d &a, const Vector3d &b, const Vector3d &c, int triangleIndex)
        {
            Vector3d ka = a, kb = b;
            if(ka > kb)
            {
                std::swap(ka, kb);
            }
            edgeToTriangles[{ ka, kb }].push_back({ c, triangleIndex });
        };

        const uint32_t triangleCount = input.GetSize() / 3;
        for(uint32_t f = 0; f < triangleCount; ++f)
        {
            const Vector3d& p0 = input[3 * f + 0];
            const Vector3d& p1 = input[3 * f + 1];
            const Vector3d& p2 = input[3 * f + 2];
            HandleEdge(p0, p1, p2, f);
            HandleEdge(p1, p2, p0, f);
            HandleEdge(p2, p0, p1, f);
        }

        std::vector<std::vector<int>> result(triangleCount);
        for(auto &[edge, records] : edgeToTriangles)
        {
            for(uint32_t i = 0, iEnd = records.size() - 1; i < iEnd; ++i)
            {
                for(uint32_t j = i + 1; j < records.size(); ++j)
                {
                    if(CantHaveInterestingIntersections(
                        edge.first, edge.second, records[i].oppositeVertex, records[j].oppositeVertex))
                    {
                        const auto [a, b] = std::minmax(records[i].triangleIndex, records[j].triangleIndex);
                        result[a].push_back(b);
                    }
                }
            }
        }

        return result;
    }

    Expansion4 ResolveSymbolicIntersection(
        Vector3d a0, Vector3d a1, Vector3d a2,
        Vector3d b0, Vector3d b1, Vector3d b2,
        SymbolicTriangleTriangleIntersection<double>::Element elemA,
        SymbolicTriangleTriangleIntersection<double>::Element elemB)
    {
        using enum SI::Element;

        if(std::to_underlying(elemA) > std::to_underlying(elemB))
        {
            std::swap(elemA, elemB);
            std::swap(a0, b0);
            std::swap(a1, b1);
            std::swap(a2, b2);
        }
        assert(elemA != F);

        if(elemA == V0) { return Expansion4(Vector4d(a0, 1)); }
        if(elemA == V1) { return Expansion4(Vector4d(a1, 1)); }
        if(elemA == V2) { return Expansion4(Vector4d(a2, 1)); }

        const Vector3d p = elemA == E01 ? a0 : elemA == E12 ? a1 : a2;
        const Vector3d q = elemA == E01 ? a1 : elemA == E12 ? a2 : a0;
        if(elemB == F)
        {
            return IntersectLineTriangle3D(p, q, b0, b1, b2);
        }

        const Vector3d m = elemB == E01 ? b0 : elemB == E12 ? b1 : b2;
        const Vector3d n = elemB == E01 ? b1 : elemB == E12 ? b2 : b0;
        return IntersectLine3D(p, q, m, n);
    }

    struct TrianglePairIntersection
    {
        uint32_t triangleA;
        uint32_t triangleB;
        SI intersection;
        std::vector<Expansion4> points;
    };

    struct PerTriangleOutput
    {
        std::vector<Expansion4> points;
        std::vector<Vector3i> triangles;
        std::vector<Vector2i> cutEdges;
    };

    struct TrianglePairIntersections
    {
        Span<std::vector<TrianglePairIntersection>> intersections;

        uint32_t GetPairwiseIntersectionCount(uint32_t triangle) const
        {
            return intersections[triangle].size();
        }

        const TrianglePairIntersection& GetPairwiseIntersection(uint32_t triangleA, uint32_t index) const
        {
            return intersections[triangleA][index];
        }
    };

    struct IndexedTrianglePairIntersections
    {
        Span<std::vector<int>> indirectIndices;
        Span<TrianglePairIntersection> intersections;

        uint32_t GetPairwiseIntersectionCount(uint32_t triangle) const
        {
            return indirectIndices[triangle].size();
        }

        const TrianglePairIntersection &GetPairwiseIntersection(uint32_t triangleA, uint32_t index) const
        {
            const int intersectionIndex = indirectIndices[triangleA][index];
            return intersections[intersectionIndex];
        }
    };

    template<typename PairwiseIntersections>
    void RefineTriangles(
        const IndexedPositions<double> &inputPositions,
        bool                            collectCutEdges,
        Span<uint8_t>                   degenerateTriangleFlags,
        const PairwiseIntersections    &pairwiseIntersections,
        MutSpan<PerTriangleOutput>      triangleToOutput)
    {
        const uint32_t triangleCount = inputPositions.GetSize() / 3;
        assert(triangleCount == degenerateTriangleFlags.size());
        assert(triangleCount == triangleToOutput.size());

        RTRC_MESH_COREFINEMENT_PARALLEL_FOR<uint32_t>(0, triangleCount, [&](uint32_t triangleA)
        {
            const Vector3d &p0 = inputPositions[3 * triangleA + 0];
            const Vector3d &p1 = inputPositions[3 * triangleA + 1];
            const Vector3d &p2 = inputPositions[3 * triangleA + 2];
            auto &output = triangleToOutput[triangleA];

            if(degenerateTriangleFlags[triangleA]) // Degenerate triangle is directly copied to output
            {
                output.points.emplace_back(Vector4d(p0, 1));
                output.points.emplace_back(Vector4d(p1, 1));
                output.points.emplace_back(Vector4d(p2, 1));
                output.triangles.emplace_back(0, 1, 2);
                return;
            }

            const uint32_t symbolicIntersectionCount = pairwiseIntersections.GetPairwiseIntersectionCount(triangleA);
            if(symbolicIntersectionCount == 0)
            {
                output.points =
                {
                    Expansion4(Expansion(p0.x), Expansion(p0.y), Expansion(p0.z), Expansion(1.0)),
                    Expansion4(Expansion(p1.x), Expansion(p1.y), Expansion(p1.z), Expansion(1.0)),
                    Expansion4(Expansion(p2.x), Expansion(p2.y), Expansion(p2.z), Expansion(1.0)),
                };
                output.triangles.emplace_back(0, 1, 2);
                return;
            }

            std::vector<Expansion4> inputPoints;
            std::map<Expansion4, int, CompareHomogeneousPoint> inputPointMap;
            auto AddInputPoint = [&](const Expansion4 &p)
            {
                if(auto it = inputPointMap.find(p); it != inputPointMap.end())
                {
                    return it->second;
                }
                const int result = static_cast<int>(inputPoints.size());
                inputPoints.push_back(p);
                inputPointMap.insert({ p, result });
                return result;
            };

            std::vector<CDT2D::Constraint> inputConstraints;

            // Add triangle vertices as input points and triangle edges as input constraints
            {
                const int i0 = AddInputPoint(Expansion4(Vector4d(p0, 1)));
                const int i1 = AddInputPoint(Expansion4(Vector4d(p1, 1)));
                const int i2 = AddInputPoint(Expansion4(Vector4d(p2, 1)));
                inputConstraints.emplace_back(i0, i1, 0u);
                inputConstraints.emplace_back(i1, i2, 0u);
                inputConstraints.emplace_back(i2, i0, 0u);
            }

            // Add points and constraints from triangle intersections
            for(uint32_t infoIndex = 0; infoIndex < symbolicIntersectionCount; ++infoIndex)
            {
                const TrianglePairIntersection &intersectionInfo =
                    pairwiseIntersections.GetPairwiseIntersection(triangleA, infoIndex);

                auto &evalulatedPoints = intersectionInfo.points;
                switch(intersectionInfo.intersection.GetType())
                {
                case SI::Type::None:
                {
                    Unreachable();
                }
                case SI::Type::Point:
                {
                    assert(evalulatedPoints.size() == 1);
                    AddInputPoint(evalulatedPoints[0]);
                    break;
                }
                case SI::Type::Edge:
                {
                    assert(evalulatedPoints.size() == 2);
                    const int i0 = AddInputPoint(evalulatedPoints[0]);
                    const int i1 = AddInputPoint(evalulatedPoints[1]);
                    inputConstraints.emplace_back(i0, i1, 1u);
                    break;
                }
                case SI::Type::Polygon:
                {
                    StaticVector<int, 6> is;
                    for(auto &p : evalulatedPoints)
                    {
                        is.push_back(AddInputPoint(p));
                    }
                    for(uint32_t i = 0; i < is.size(); ++i)
                    {
                        inputConstraints.emplace_back(is[i], is[(i + 1) % is.size()], 1u);
                    }
                    break;
                }
                }
            }

            // Project onto plane

            std::vector<Expansion3> inputPoints2D;
            const int projectAxis = FindMaximalNormalAxis(p0, p1, p2);
            inputPoints2D.reserve(inputPoints.size());
            for(auto &p : inputPoints)
            {
                inputPoints2D.emplace_back(p[(projectAxis + 1) % 3], p[(projectAxis + 2) % 3], p.w);
            }

            // Perform constrained triangulation

            CDT2D cdt;
            cdt.delaunay = true;
            cdt.trackConstraintMask = collectCutEdges;
            cdt.Triangulate(inputPoints2D, inputConstraints);

            output.points = std::move(inputPoints);
            output.triangles = std::move(cdt.triangles);

            if(collectCutEdges)
            {
                for(auto &pair : cdt.edgeToConstraintMask)
                {
                    if(pair.second > 0)
                    {
                        output.cutEdges.emplace_back(pair.first.first, pair.first.second);
                    }
                }
            }

            auto Get2DPoint = [&](uint32_t v) -> const Expansion3 &
            {
                if(v < inputPoints2D.size())
                {
                    return inputPoints2D[v];
                }
                return cdt.newIntersections[v - inputPoints2D.size()].position;
            };
            const int orientBefore = Orient2DHomogeneous(inputPoints2D[0], inputPoints2D[1], inputPoints2D[2]);
            const int orientAfter = Orient2DHomogeneous(Get2DPoint(output.triangles[0][0]),
                                                        Get2DPoint(output.triangles[0][1]),
                                                        Get2DPoint(output.triangles[0][2]));
            if(orientBefore != orientAfter)
            {
                for(Vector3i& triangle : output.triangles)
                {
                    std::swap(triangle[0], triangle[2]);
                }
            }

            // Resolve new intersections encountered in triangulation

            for(const CDT2D::ConstraintIntersection &cinct : cdt.newIntersections)
            {
                const auto [a, b, mask0] = inputConstraints[cinct.constraint0];
                const auto [c, d, mask1] = inputConstraints[cinct.constraint1];
                output.points.push_back(IntersectLine3D(
                    output.points[a], output.points[b], output.points[c], output.points[d], projectAxis));
            }
        });
    }

    void ComputeSymbolicIntersection(
        uint32_t                               triangleA,
        uint32_t                               triangleB,
        const IndexedPositions<double>        &positionsA,
        const IndexedPositions<double>        &positionsB,
        std::vector<TrianglePairIntersection> &output)
    {
        const Vector3d &a0 = positionsA[3 * triangleA + 0];
        const Vector3d &a1 = positionsA[3 * triangleA + 1];
        const Vector3d &a2 = positionsA[3 * triangleA + 2];
        const Vector3d &b0 = positionsB[3 * triangleB + 0];
        const Vector3d &b1 = positionsB[3 * triangleB + 1];
        const Vector3d &b2 = positionsB[3 * triangleB + 2];

        const auto intersection = SI::Intersect(a0, a1, a2, b0, b1, b2, true);
        if(intersection.GetPoints().IsEmpty())
        {
            return;
        }

        output.push_back({ triangleA, triangleB, intersection, {} });
    }

    void GenerateRoundedOutput(
        Span<PerTriangleOutput>         triangleToOutput,
        const IndexedPositions<double> &inputPositions,
        std::vector<Vector3d>          &outputPositions,
        std::vector<uint32_t>          &outputIndices,
        std::vector<uint32_t>          *outputFaceToInputFace,
        std::vector<Vector2u>          *outputCutEdges)
    {
        std::map<Vector3d, uint32_t> positionToIndex;
        auto GetIndex = [&](const Vector3d &position)
        {
            if(auto it = positionToIndex.find(position); it != positionToIndex.end())
            {
                return it->second;
            }
            const uint32_t newIndex = static_cast<uint32_t>(outputPositions.size());
            positionToIndex.insert({ position, newIndex });
            outputPositions.push_back(position);
            return newIndex;
        };

        std::vector<uint32_t> localIndexToGlobalIndex;
        for(uint32_t triangleIndex = 0; triangleIndex < triangleToOutput.size(); ++triangleIndex)
        {
            const PerTriangleOutput &newTriangles = triangleToOutput[triangleIndex];

            if(outputCutEdges)
            {
                localIndexToGlobalIndex.clear();
                localIndexToGlobalIndex.resize(newTriangles.points.size(), UINT32_MAX);
            }

            std::vector<Vector3d> roundedPoints;
            for(uint32_t i = 0; i < 3; ++i) // Copy the first three points directly from the input.
            {
                roundedPoints.push_back(inputPositions[3 * triangleIndex + i]);
            }
            for(uint32_t i = 3; i < newTriangles.points.size(); ++i)
            {
                const Expansion4 &p = newTriangles.points[i];
                const auto w = p.w.ToWord();
                roundedPoints.emplace_back(
                    static_cast<double>(p.x.ToWord() / w),
                    static_cast<double>(p.y.ToWord() / w),
                    static_cast<double>(p.z.ToWord() / w));
            }

            for(const Vector3i &triangle : newTriangles.triangles)
            {
                const int localIndex0 = triangle[0];
                const int localIndex1 = triangle[1];
                const int localIndex2 = triangle[2];

                const Vector3d &p0 = roundedPoints[localIndex0];
                const Vector3d &p1 = roundedPoints[localIndex1];
                const Vector3d &p2 = roundedPoints[localIndex2];

                const uint32_t globalIndex0 = GetIndex(p0);
                const uint32_t globalIndex1 = GetIndex(p1);
                const uint32_t globalIndex2 = GetIndex(p2);

                if(outputCutEdges)
                {
                    localIndexToGlobalIndex[localIndex0] = globalIndex0;
                    localIndexToGlobalIndex[localIndex1] = globalIndex1;
                    localIndexToGlobalIndex[localIndex2] = globalIndex2;
                }

                outputIndices.push_back(globalIndex0);
                outputIndices.push_back(globalIndex1);
                outputIndices.push_back(globalIndex2);

                if(outputFaceToInputFace)
                {
                    outputFaceToInputFace->push_back(triangleIndex);
                }
            }

            if(outputCutEdges)
            {
                for(auto &edge : newTriangles.cutEdges)
                {
                    uint32_t v0 = localIndexToGlobalIndex[edge.x];
                    uint32_t v1 = localIndexToGlobalIndex[edge.y];
                    assert(v0 != UINT32_MAX && v1 != UINT32_MAX);
                    if(v0 > v1)
                    {
                        std::swap(v0, v1);
                    }
                    outputCutEdges->emplace_back(v0, v1);
                }
            }
        }
    }

    void GenerateExactOutput(
        Span<PerTriangleOutput>  triangleToOutput,
        std::vector<Expansion4> &outputPositions,
        std::vector<uint32_t>   &outputIndices,
        std::vector<uint32_t>   *outputFaceToInputFace,
        std::vector<Vector2u>   *outputCutEdges)
    {
        std::map<Expansion4, uint32_t, CompareHomogeneousPoint> positionToIndex;
        auto GetIndex = [&](const Expansion4 &position)
        {
            if(auto it = positionToIndex.find(position); it != positionToIndex.end())
            {
                return it->second;
            }
            const uint32_t newIndex = static_cast<uint32_t>(outputPositions.size());
            positionToIndex.insert({ position, newIndex });
            outputPositions.push_back(position);
            return newIndex;
        };

        std::vector<uint32_t> localIndexToGlobalIndex;
        for(uint32_t triangleIndex = 0; triangleIndex < triangleToOutput.size(); ++triangleIndex)
        {
            const PerTriangleOutput &newTriangles = triangleToOutput[triangleIndex];

            if(outputCutEdges)
            {
                localIndexToGlobalIndex.clear();
                localIndexToGlobalIndex.resize(newTriangles.points.size(), UINT32_MAX);
            }

            for(const Vector3i &triangle : newTriangles.triangles)
            {
                const int localIndex0 = triangle[0];
                const int localIndex1 = triangle[1];
                const int localIndex2 = triangle[2];

                const Expansion4 &p0 = newTriangles.points[localIndex0];
                const Expansion4 &p1 = newTriangles.points[localIndex1];
                const Expansion4 &p2 = newTriangles.points[localIndex2];

                const uint32_t globalIndex0 = GetIndex(p0);
                const uint32_t globalIndex1 = GetIndex(p1);
                const uint32_t globalIndex2 = GetIndex(p2);

                if(outputCutEdges)
                {
                    localIndexToGlobalIndex[localIndex0] = globalIndex0;
                    localIndexToGlobalIndex[localIndex1] = globalIndex1;
                    localIndexToGlobalIndex[localIndex2] = globalIndex2;
                }

                outputIndices.push_back(globalIndex0);
                outputIndices.push_back(globalIndex1);
                outputIndices.push_back(globalIndex2);

                if(outputFaceToInputFace)
                {
                    outputFaceToInputFace->push_back(triangleIndex);
                }
            }

            if(outputCutEdges)
            {
                for(auto &edge : newTriangles.cutEdges)
                {
                    uint32_t v0 = localIndexToGlobalIndex[edge.x];
                    uint32_t v1 = localIndexToGlobalIndex[edge.y];
                    assert(v0 != UINT32_MAX && v1 != UINT32_MAX);
                    if(v0 > v1)
                    {
                        std::swap(v0, v1);
                    }
                    outputCutEdges->emplace_back(v0, v1);
                }
            }
        }
    }

} // namespace CorefineDetail

void MeshCorefinement::Corefine(
    Span<Vector3d> inputPositionsA, Span<uint32_t> inputIndicesA,
    Span<Vector3d> inputPositionsB, Span<uint32_t> inputIndicesB)
{
    using namespace CorefineDetail;

    const IndexedPositions inputA(inputPositionsA, inputIndicesA);
    const IndexedPositions inputB(inputPositionsB, inputIndicesB);
    assert(inputA.GetSize() % 3 == 0);
    assert(inputB.GetSize() % 3 == 0);

    const uint32_t triangleCountA = inputA.GetSize() / 3;
    const uint32_t triangleCountB = inputB.GetSize() / 3;

    outputPositionsA.clear();
    outputPositionsB.clear();
    outputExactPositionsA.clear();
    outputExactPositionsB.clear();
    outputIndicesA.clear();
    outputIndicesB.clear();
    outputFaceMapA.clear();
    outputFaceMapB.clear();
    outputCutEdgesA.clear();
    outputCutEdgesB.clear();

    // Filter degenerate triangles

    std::vector<uint8_t> degenerateTriangleFlagA(triangleCountA);
    RTRC_MESH_COREFINEMENT_PARALLEL_FOR<uint32_t>(0, triangleCountA, [&](uint32_t a)
    {
        const Vector3d &p0 = inputA[3 * a + 0];
        const Vector3d &p1 = inputA[3 * a + 1];
        const Vector3d &p2 = inputA[3 * a + 2];
        degenerateTriangleFlagA[a] = AreCoLinear(p0, p1, p2);
    });

    std::vector<uint8_t> degenerateTriangleFlagB(triangleCountB);
    RTRC_MESH_COREFINEMENT_PARALLEL_FOR<uint32_t>(0, triangleCountB, [&](uint32_t b)
    {
        const Vector3d &p0 = inputB[3 * b + 0];
        const Vector3d &p1 = inputB[3 * b + 1];
        const Vector3d &p2 = inputB[3 * b + 2];
        degenerateTriangleFlagB[b] = AreCoLinear(p0, p1, p2);
    });

    // Compute triangle-triangle symbolic intersections

    BVH<double> bvhB;
    {
        std::vector<AABB3d> primitiveBoundingsB;
        primitiveBoundingsB.reserve(triangleCountB);
        for(uint32_t f = 0; f < triangleCountB; ++f)
        {
            AABB3d bbox;
            bbox |= inputB[3 * f + 0];
            bbox |= inputB[3 * f + 1];
            bbox |= inputB[3 * f + 2];
            primitiveBoundingsB.push_back(bbox);
        }
        bvhB = BVH<double>::Build(primitiveBoundingsB);
    }

    std::vector<std::vector<TrianglePairIntersection>> triangleAToPairwiseIntersections(triangleCountA);

    RTRC_MESH_COREFINEMENT_PARALLEL_FOR<uint32_t>(0, triangleCountA, [&](uint32_t triangleA)
    {
        if(degenerateTriangleFlagA[triangleA])
        {
            return;
        }

        AABB3d triangleBoundingBox;
        triangleBoundingBox |= inputA[3 * triangleA + 0];
        triangleBoundingBox |= inputA[3 * triangleA + 1];
        triangleBoundingBox |= inputA[3 * triangleA + 2];

        bvhB.TraversalPrimitives(
            [&](const AABB3d &targetBoundingBox)
            {
                return triangleBoundingBox.Intersect(targetBoundingBox);
            },
            [&](uint32_t triangleB)
            {
                if(!degenerateTriangleFlagB[triangleB])
                {
                    ComputeSymbolicIntersection(
                        triangleA, triangleB, inputA, inputB, triangleAToPairwiseIntersections[triangleA]);
                }
            });
    });

    // Resolve symbolic intersections

    RTRC_MESH_COREFINEMENT_PARALLEL_FOR<uint32_t>(0, triangleCountA, [&](uint32_t triangleA)
    {
        auto& symbolicIntersections = triangleAToPairwiseIntersections[triangleA];
        if(symbolicIntersections.empty())
        {
            return;
        }

        const Vector3d &p0 = inputA[3 * triangleA + 0];
        const Vector3d &p1 = inputA[3 * triangleA + 1];
        const Vector3d &p2 = inputA[3 * triangleA + 2];

        for(TrianglePairIntersection &intersectionInfo : symbolicIntersections)
        {
            const uint32_t triangleB = intersectionInfo.triangleB;
            const Vector3d &q0 = inputB[3 * triangleB + 0];
            const Vector3d &q1 = inputB[3 * triangleB + 1];
            const Vector3d &q2 = inputB[3 * triangleB + 2];

            intersectionInfo.points.reserve(intersectionInfo.intersection.GetPoints().size());
            for(auto &inct : intersectionInfo.intersection.GetPoints())
            {
                const SI::Element elemA = inct.GetElement0();
                const SI::Element elemB = inct.GetElement1();
                intersectionInfo.points.push_back(ResolveSymbolicIntersection(p0, p1, p2, q0, q1, q2, elemA, elemB));

                Expansion4 &newPoint = intersectionInfo.points.back();
                newPoint.x.Compress();
                newPoint.y.Compress();
                newPoint.z.Compress();
                newPoint.w.Compress();
            }
        }
    });

    // Retriangulate A

    std::vector<PerTriangleOutput> triangleAToOutput(triangleCountA);
    RefineTriangles(
        inputA, trackCutEdges, degenerateTriangleFlagA,
        TrianglePairIntersections{ triangleAToPairwiseIntersections }, triangleAToOutput);

    // Collect symbolic intersections for B

    std::vector<std::vector<TrianglePairIntersection>> triangleBToPairwiseIntersections(triangleCountB);
    for(auto &incts : triangleAToPairwiseIntersections)
    {
        for(auto &inct : incts)
        {
            const uint32_t triangleB = inct.triangleB;
            triangleBToPairwiseIntersections[triangleB].push_back(std::move(inct));
        }
    }

    // Retriangulate B

    std::vector<PerTriangleOutput> triangleBToOutput(triangleCountB);
    RefineTriangles(
        inputB, trackCutEdges, degenerateTriangleFlagB,
        TrianglePairIntersections{ triangleBToPairwiseIntersections }, triangleBToOutput);

    // Final rounding

    if(preserveExactPositions)
    {
        GenerateExactOutput(
            triangleAToOutput, outputExactPositionsA, outputIndicesA,
            trackFaceMap ? &outputFaceMapA : nullptr,
            trackCutEdges ? &outputCutEdgesA : nullptr);
        GenerateExactOutput(
            triangleBToOutput, outputExactPositionsB, outputIndicesB,
            trackFaceMap ? &outputFaceMapB : nullptr,
            trackCutEdges ? &outputCutEdgesB : nullptr);
    }
    else
    {
        GenerateRoundedOutput(
            triangleAToOutput, inputA, outputPositionsA, outputIndicesA,
            trackFaceMap ? &outputFaceMapA : nullptr,
            trackCutEdges ? &outputCutEdgesA : nullptr);
        GenerateRoundedOutput(
            triangleBToOutput, inputB, outputPositionsB, outputIndicesB,
            trackFaceMap ? &outputFaceMapB : nullptr,
            trackCutEdges ? &outputCutEdgesB : nullptr);
    }

    if(trackCutEdges)
    {
        std::ranges::sort(outputCutEdgesA);
        auto uniqueA = std::ranges::unique(outputCutEdgesA);
        outputCutEdgesA.erase(uniqueA.begin(), uniqueA.end());

        std::ranges::sort(outputCutEdgesB);
        auto uniqueB = std::ranges::unique(outputCutEdgesB);
        outputCutEdgesB.erase(uniqueB.begin(), uniqueB.end());
    }
}

void MeshSelfIntersectionRefinement::Refine(Span<Vector3d> inputPositions, Span<uint32_t> inputIndices)
{
    using namespace CorefineDetail;

    const IndexedPositions input(inputPositions, inputIndices);
    assert(input.GetSize() % 3 == 0);
    const uint32_t triangleCount = input.GetSize() / 3;

    outputPositions.clear();
    outputExactPositions.clear();
    outputIndices.clear();
    outputFaceMap.clear();
    outputCutEdges.clear();

    // Filter degenerate triangles

    std::vector<uint8_t> degenerateTriangleFlags(triangleCount);
    RTRC_MESH_COREFINEMENT_PARALLEL_FOR<uint32_t>(0, triangleCount, [&](uint32_t triangle)
    {
        const Vector3d &p0 = input[3 * triangle + 0];
        const Vector3d& p1 = input[3 * triangle + 1];
        const Vector3d& p2 = input[3 * triangle + 2];
        degenerateTriangleFlags[triangle] = AreCoLinear(p0, p1, p2);
    });

    // BVH for all triangles

    BVH<double> bvh;
    std::vector<AABB3d> triangleBoundingBoxes;
    {
        triangleBoundingBoxes.reserve(triangleCount);
        for(uint32_t f = 0; f < triangleCount; ++f)
        {
            AABB3d bbox;
            bbox |= input[3 * f + 0];
            bbox |= input[3 * f + 1];
            bbox |= input[3 * f + 2];
            triangleBoundingBoxes.push_back(bbox);
        }
        bvh = BVH<double>::Build(triangleBoundingBoxes);
    }

    // Adjacent triangles are guaranteed to intersect. However, computing these intersections is
    // time-consuming and doesn't contribute to the final output. Identify such cases here and
    // exclude these triangles from intersection calculations in subsequent processes.

    const std::vector<std::vector<int>> adjacentLists = DetectAdjacentTriangles(input);

    // Triangle-triangle symbolic intersections

    std::vector<std::vector<TrianglePairIntersection>> tempTriangleToIntersections(triangleCount);
    RTRC_MESH_COREFINEMENT_PARALLEL_FOR<uint32_t>(0, triangleCount, [&](uint32_t triangleA)
    {
        if(degenerateTriangleFlags[triangleA])
        {
            return;
        }

        bvh.TraversalPrimitives(
            [&](const AABB3d& targetBoundingBox)
            {
                return triangleBoundingBoxes[triangleA].Intersect(targetBoundingBox);
            },
            [&](uint32_t triangleB)
            {
                if(triangleB <= triangleA || degenerateTriangleFlags[triangleB])
                {
                    return;
                }
                for(int adjacentTriangle : adjacentLists[triangleA])
                {
                    if(adjacentTriangle == static_cast<int>(triangleB))
                    {
                        return;
                    }
                }
                ComputeSymbolicIntersection(triangleA, triangleB, input, input, tempTriangleToIntersections[triangleA]);
            });
    });

    std::vector<TrianglePairIntersection> allIntersections;
    std::vector<std::vector<int>> triangleToIntersectionIndices(triangleCount);
    for(auto &intersections : tempTriangleToIntersections)
    {
        for(auto &intersection : intersections)
        {
            const int intersectionIndex = static_cast<int>(allIntersections.size());
            triangleToIntersectionIndices[intersection.triangleA].push_back(intersectionIndex);
            triangleToIntersectionIndices[intersection.triangleB].push_back(intersectionIndex);
            allIntersections.push_back(std::move(intersection));
        }
    }

    // Resolve symbolic intersections

    RTRC_MESH_COREFINEMENT_PARALLEL_FOR<uint32_t>(0, allIntersections.size(), [&](uint32_t intersectionIndex)
    {
        auto &symbolicIntersections = allIntersections[intersectionIndex];

        const Vector3d &a0 = input[3 * symbolicIntersections.triangleA + 0];
        const Vector3d &a1 = input[3 * symbolicIntersections.triangleA + 1];
        const Vector3d &a2 = input[3 * symbolicIntersections.triangleA + 2];
        
        const Vector3d &b0 = input[3 * symbolicIntersections.triangleB + 0];
        const Vector3d &b1 = input[3 * symbolicIntersections.triangleB + 1];
        const Vector3d &b2 = input[3 * symbolicIntersections.triangleB + 2];

        symbolicIntersections.points.reserve(symbolicIntersections.intersection.GetPoints().size());
        for(auto &intersection : symbolicIntersections.intersection.GetPoints())
        {
            const SI::Element elemA = intersection.GetElement0();
            const SI::Element elemB = intersection.GetElement1();
            symbolicIntersections.points.push_back(ResolveSymbolicIntersection(a0, a1, a2, b0, b1, b2, elemA, elemB));

            Expansion4 &newPoint = symbolicIntersections.points.back();
            newPoint.x.Compress();
            newPoint.y.Compress();
            newPoint.z.Compress();
            newPoint.w.Compress();
        }
    });

    // Constrained triangulation

    std::vector<PerTriangleOutput> triangleToOutput(triangleCount);
    RefineTriangles(
        input, trackCutEdges, degenerateTriangleFlags,
        IndexedTrianglePairIntersections{ triangleToIntersectionIndices, allIntersections },
        triangleToOutput);

    // Final rounding

    if(preserveExactPositions)
    {
        GenerateExactOutput(
            triangleToOutput, outputExactPositions, outputIndices,
            trackFaceMap ? &outputFaceMap : nullptr,
            trackCutEdges ? &outputCutEdges : nullptr);
    }
    else
    {
        GenerateRoundedOutput(
            triangleToOutput, input, outputPositions, outputIndices,
            trackFaceMap ? &outputFaceMap : nullptr,
            trackCutEdges ? &outputCutEdges : nullptr);
    }

    if(trackCutEdges)
    {
        std::ranges::sort(outputCutEdges);
        auto unique = std::ranges::unique(outputCutEdges);
        outputCutEdges.erase(unique.begin(), unique.end());
    }
}

RTRC_GEO_END
