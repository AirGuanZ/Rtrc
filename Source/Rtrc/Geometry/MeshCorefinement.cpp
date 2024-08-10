#include <map>
#include <set>

#include <bvh/bvh.hpp>
#include <bvh/leaf_collapser.hpp>
#include <bvh/locally_ordered_clustering_builder.hpp>

#include <Rtrc/Core/Math/AABB.h>
#include <Rtrc/Core/Parallel.h>
#include <Rtrc/Core/Unreachable.h>
#include <Rtrc/Geometry/ConstrainedTriangulation.h>
#include <Rtrc/Geometry/Exact/Intersection.h>
#include <Rtrc/Geometry/Exact/Predicates.h>
#include <Rtrc/Geometry/MeshCorefinement.h>
#include <Rtrc/Geometry/TriangleTriangleIntersection.h>
#include <Rtrc/Geometry/Utility.h>

#if RTRC_DEBUG
#define RTRC_MESH_COREFINEMENT_PARALLEL_FOR Rtrc::ParallelForDebug
#else
#define RTRC_MESH_COREFINEMENT_PARALLEL_FOR Rtrc::ParallelFor
#endif

RTRC_GEO_BEGIN

namespace CorefineDetail
{

    bvh::Vector3<double> ConvertVector3(const Vector3d &v)
    {
        return { v.x, v.y, v.z };
    }

    bvh::BoundingBox<double> ConvertBoundingBox(const AABB3d &boundingBox)
    {
        return { ConvertVector3(boundingBox.lower), ConvertVector3(boundingBox.upper) };
    }

    // Helper class for collecting potentially intersected triangles
    class BVH
    {
    public:

        explicit BVH(const IndexedPositions<double> &positions)
        {
            assert(positions.GetSize() % 3 == 0);
            const uint32_t triangleCount = positions.GetSize() / 3;

            AABB3d globalBoundingBox;
            std::vector<bvh::BoundingBox<double>> primitiveBoundingBoxes(triangleCount);
            std::vector<bvh::Vector3<double>> primitiveCenters(triangleCount);
            for(uint32_t i = 0, j = 0; i < triangleCount; ++i, j += 3)
            {
                AABB3d primitiveBoundingBox;
                primitiveBoundingBox |= positions[j + 0];
                primitiveBoundingBox |= positions[j + 1];
                primitiveBoundingBox |= positions[j + 2];
                primitiveBoundingBoxes[i] = ConvertBoundingBox(primitiveBoundingBox);

                globalBoundingBox |= primitiveBoundingBox;

                const Vector3d center = 1.0 / 3 * (positions[j + 0] + positions[j + 1] + positions[j + 2]);
                primitiveCenters[i] = ConvertVector3(center);
            }

            bvh::Bvh<double> tree;
            bvh::LocallyOrderedClusteringBuilder<bvh::Bvh<double>, uint32_t> builder(tree);

            builder.build(
                ConvertBoundingBox(globalBoundingBox),
                primitiveBoundingBoxes.data(), primitiveCenters.data(),
                triangleCount);

            bvh::LeafCollapser(tree).collapse();

            nodes_.resize(tree.node_count);
            for(uint32_t ni = 0; ni < tree.node_count; ++ni)
            {
                auto &src = tree.nodes[ni];
                auto &dst = nodes_[ni];

                dst.boundingBox.lower.x = src.bounds[0];
                dst.boundingBox.lower.y = src.bounds[2];
                dst.boundingBox.lower.z = src.bounds[4];
                dst.boundingBox.upper.x = src.bounds[1];
                dst.boundingBox.upper.y = src.bounds[3];
                dst.boundingBox.upper.z = src.bounds[5];

                if(src.is_leaf())
                {
                    const size_t triangleBegin = originalTriangleIndices_.size();
                    for(size_t i = src.first_child_or_primitive;
                        i < src.first_child_or_primitive + src.primitive_count; ++i)
                    {
                        originalTriangleIndices_.push_back(static_cast<uint32_t>(tree.primitive_indices[i]));
                    }
                    const size_t triangleEnd = originalTriangleIndices_.size();

                    assert(triangleEnd != triangleBegin);
                    dst.childOffset = static_cast<uint32_t>(triangleBegin);
                    dst.childCount = static_cast<uint32_t>(triangleEnd - triangleBegin);
                }
                else
                {
                    dst.childOffset = static_cast<uint32_t>(src.first_child_or_primitive);
                    dst.childCount = 0;
                }
            }
        }

        // Functor requirement:
        //     bool IntersectBox(AABB3d boundingBox);
        //     void addCandidate(uint32_t triangleIndex);
        template<typename IntersectBoxFunc, typename AddCandidateFunc>
        void CollectCandidates(const IntersectBoxFunc &intersectBox, const AddCandidateFunc &addCandidate) const
        {
            constexpr int STACK_SIZE = 100;
            uint32_t stack[STACK_SIZE];
            int top = 0;
            stack[top++] = 0;

            while(top)
            {
                const uint32_t nodeIndex = stack[--top];
                const Node &node = nodes_[nodeIndex];

                if(node.childCount != 0)
                {
                    for(uint32_t i = node.childOffset; i < node.childOffset + node.childCount;  ++i)
                    {
                        addCandidate(originalTriangleIndices_[i]);
                    }
                    continue;
                }

                assert(top + 2 < STACK_SIZE && "BVH stack overflow!");
                if(intersectBox(nodes_[node.childOffset].boundingBox))
                {
                    stack[top++] = node.childOffset;
                }
                if(intersectBox(nodes_[node.childOffset + 1].boundingBox))
                {
                    stack[top++] = node.childOffset + 1;
                }
            }
        }

    private:

        struct Node
        {
            AABB3d boundingBox;
            uint32_t childOffset;
            uint32_t childCount; // 0 for interior nodes
        };

        std::vector<Node>     nodes_;
        std::vector<uint32_t> originalTriangleIndices_;
    };

    using SI = SymbolicTriangleTriangleIntersection<double>;

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

    void RefineTriangles(
        const IndexedPositions<double>             &inputPositions,
        bool                                        collectCutEdges,
        Span<uint8_t>                               degenerateTriangleFlags,
        Span<std::vector<TrianglePairIntersection>> triangleToPairwiseIntersections,
        MutSpan<PerTriangleOutput>                  triangleToOutput)
    {
        const uint32_t triangleCount = inputPositions.GetSize() / 3;
        assert(triangleCount == degenerateTriangleFlags.size());
        assert(triangleCount == triangleToPairwiseIntersections.size());
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

            auto& symbolicIntersections = triangleToPairwiseIntersections[triangleA];
            if(symbolicIntersections.empty())
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
            for(const TrianglePairIntersection &intersectionInfo : symbolicIntersections)
            {
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

    const BVH bvhB = BVH(inputB);
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

        bvhB.CollectCandidates(
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
        inputA, trackCutEdges, degenerateTriangleFlagA, triangleAToPairwiseIntersections, triangleAToOutput);

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
        inputB, trackCutEdges, degenerateTriangleFlagB, triangleBToPairwiseIntersections, triangleBToOutput);

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

RTRC_GEO_END
