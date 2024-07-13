#include <map>

#include <bvh/bvh.hpp>
#include <bvh/leaf_collapser.hpp>
#include <bvh/locally_ordered_clustering_builder.hpp>

#include <Rtrc/Core/Math/AABB.h>
#include <Rtrc/Core/Parallel.h>
#include <Rtrc/Core/Unreachable.h>
#include <Rtrc/Geometry/ConstrainedTriangulation.h>
#include <Rtrc/Geometry/Exact/Intersection.h>
#include <Rtrc/Geometry/MeshCorefinement.h>
#include <Rtrc/Geometry/TriangleTriangleIntersection.h>

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

    struct CompareHomogeneousPoint
    {
        bool operator()(const Expansion3 &a, const Expansion3 &b) const
        {
            if(const int c0 = CompareHomo(a.x, a.z, b.x, b.z); c0 != 0)
            {
                return c0 < 0;
            }
            return CompareHomo(a.y, a.z, b.y, b.z) < 0;
        }

        bool operator()(const Expansion4 &a, const Expansion4 &b) const
        {
            if(const int c0 = CompareHomo(a.x, a.w, b.x, b.w); c0 != 0)
            {
                return c0 < 0;
            }
            if(const int c1 = CompareHomo(a.y, a.w, b.y, b.w); c1 != 0)
            {
                return c1 < 0;
            }
            return CompareHomo(a.z, a.w, b.z, b.w) < 0;
        }

        static int CompareHomo(const Expansion &a, const Expansion &adown, const Expansion &b, const Expansion &bdown)
        {
            return adown.GetSign() * bdown.GetSign() * (a * bdown).Compare(b * adown);
        }
    };

    // Helper class for collecting potentially intersected triangles
    class BVH
    {
    public:

        explicit BVH(Span<Vector3d> positions)
        {
            assert(positions.size() % 3 == 0);
            const uint32_t triangleCount = positions.size() / 3;

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
                        originalTriangleIndices_.push_back(tree.primitive_indices[i]);
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

    Expansion4 EvaluateSymbolicIntersection(
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
    };

    void RefineTriangles(
        Span<Vector3d>                              inputPositions,
        Span<uint8_t>                               degenerateTriangleFlags,
        Span<std::vector<TrianglePairIntersection>> triangleToPairwiseIntersections,
        MutSpan<PerTriangleOutput>                  triangleToOutput)
    {
        const uint32_t triangleCount = inputPositions.size() / 3;
        assert(triangleCount == degenerateTriangleFlags.size());
        assert(triangleCount == triangleToPairwiseIntersections.size());
        assert(triangleCount == triangleToOutput.size());

        ParallelForDebug<uint32_t>(0, triangleCount, [&](uint32_t triangleA)
        {
            if(degenerateTriangleFlags[triangleA])
            {
                return;
            }
            const Vector3d &p0 = inputPositions[3 * triangleA + 0];
            const Vector3d &p1 = inputPositions[3 * triangleA + 1];
            const Vector3d &p2 = inputPositions[3 * triangleA + 2];

            auto &output = triangleToOutput[triangleA];

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

            std::vector<Vector2i> inputConstraints;

            // Add triangle vertices as input points and triangle edges as input constraints
            {
                const int i0 = AddInputPoint(Expansion4(Vector4d(p0, 1)));
                const int i1 = AddInputPoint(Expansion4(Vector4d(p1, 1)));
                const int i2 = AddInputPoint(Expansion4(Vector4d(p2, 1)));
                inputConstraints.emplace_back(i0, i1);
                inputConstraints.emplace_back(i1, i2);
                inputConstraints.emplace_back(i2, i0);
            }

            // Add ponits and constraints from triangle intersections
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
                    inputConstraints.emplace_back(i0, i1);
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
                        inputConstraints.emplace_back(is[i], is[(i + 1) % is.size()]);
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

            CDT2D cdt = CDT2D::Create(inputPoints2D, inputConstraints, true);
            output.points = std::move(inputPoints);
            output.triangles = std::move(cdt.triangles);

            // Resolve new intersections encountered in triangulation

            for(const CDT2D::ConstraintIntersection &cinct : cdt.newIntersections)
            {
                const auto [a, b] = inputConstraints[cinct.constraint0];
                const auto [c, d] = inputConstraints[cinct.constraint1];
                output.points.push_back(IntersectLine3D(
                    output.points[a], output.points[b], output.points[c], output.points[d], projectAxis));
            }
        });
    }

} // namespace CorefineDetail

void Corefine(
    Span<Vector3d>                     inputPositionsA,
    Span<Vector3d>                     inputPositionsB,
    std::vector<Vector3d>             &outputPositionsA,
    std::vector<Vector3d>             &outputPositionsB,
    ObserverPtr<std::vector<uint32_t>> outputFaceToInputFaceA,
    ObserverPtr<std::vector<uint32_t>> outputFaceToInputFaceB)
{
    using namespace CorefineDetail;

    assert(inputPositionsA.size() % 3 == 0);
    assert(inputPositionsB.size() % 3 == 0);
    const uint32_t triangleCountA = inputPositionsA.size() / 3;
    const uint32_t triangleCountB = inputPositionsB.size() / 3;

    outputPositionsA.clear();
    outputPositionsB.clear();
    if(outputFaceToInputFaceA)
    {
        outputFaceToInputFaceA->clear();
    }
    if(outputFaceToInputFaceB)
    {
        outputFaceToInputFaceB->clear();
    }
    if(!triangleCountA && !triangleCountB)
    {
        return;
    }

    // Filter degenerate triangles

    std::vector<uint8_t> degenerateTriangleFlagA(triangleCountA);
    ParallelForDebug<uint32_t>(0, triangleCountA, [&](uint32_t a)
    {
        const Vector3d &p0 = inputPositionsA[3 * a + 0];
        const Vector3d &p1 = inputPositionsA[3 * a + 1];
        const Vector3d &p2 = inputPositionsA[3 * a + 2];
        if(p0 == p1 || p1 == p2 || p2 == p0)
        {
            degenerateTriangleFlagA[a] = true;
        }
    });

    std::vector<uint8_t> degenerateTriangleFlagB(triangleCountB);
    ParallelForDebug<uint32_t>(0, triangleCountB, [&](uint32_t b)
    {
        const Vector3d &p0 = inputPositionsB[3 * b + 0];
        const Vector3d &p1 = inputPositionsB[3 * b + 1];
        const Vector3d &p2 = inputPositionsB[3 * b + 2];
        if(p0 == p1 || p1 == p2 || p2 == p0)
        {
            degenerateTriangleFlagA[b] = true;
        }
    });

    // Compute triangle-triangle symbolic intersections

    std::vector<std::vector<TrianglePairIntersection>> triangleAToPairwiseIntersections(triangleCountA);

    auto ComputeSymbolicIntersection = [&](uint32_t triangleA, uint32_t triangleB)
    {
        if(degenerateTriangleFlagA[triangleA] || degenerateTriangleFlagB[triangleB])
        {
            return;
        }

        const Vector3d &a0 = inputPositionsA[3 * triangleA + 0];
        const Vector3d &a1 = inputPositionsA[3 * triangleA + 1];
        const Vector3d &a2 = inputPositionsA[3 * triangleA + 2];
        const Vector3d &b0 = inputPositionsB[3 * triangleB + 0];
        const Vector3d &b1 = inputPositionsB[3 * triangleB + 1];
        const Vector3d &b2 = inputPositionsB[3 * triangleB + 2];

        const auto intersection = SI::Intersect(a0, a1, a2, b0, b1, b2, true);
        if(intersection.GetPoints().IsEmpty())
        {
            return;
        }

        triangleAToPairwiseIntersections[triangleA].push_back({ triangleA, triangleB, intersection, {} });
    };

    const BVH bvhB = BVH(inputPositionsB);

    ParallelForDebug<uint32_t>(0, triangleCountA, [&](uint32_t triangleA)
    {
        AABB3d triangleBoundingBox;
        triangleBoundingBox |= inputPositionsA[3 * triangleA + 0];
        triangleBoundingBox |= inputPositionsA[3 * triangleA + 1];
        triangleBoundingBox |= inputPositionsA[3 * triangleA + 2];

        bvhB.CollectCandidates(
            [&](const AABB3d &targetBoundingBox)
            {
                return triangleBoundingBox.Intersect(targetBoundingBox);
            },
            [&](uint32_t triangleB)
            {
                ComputeSymbolicIntersection(triangleA, triangleB);
            });
    });

    // Resolve symbolic intersections

    ParallelForDebug<uint32_t>(0, triangleCountA, [&](uint32_t triangleA)
    {
        auto& symbolicIntersections = triangleAToPairwiseIntersections[triangleA];
        if(symbolicIntersections.empty())
        {
            return;
        }

        const Vector3d &p0 = inputPositionsA[3 * triangleA + 0];
        const Vector3d &p1 = inputPositionsA[3 * triangleA + 1];
        const Vector3d &p2 = inputPositionsA[3 * triangleA + 2];

        for(TrianglePairIntersection &intersectionInfo : symbolicIntersections)
        {
            const uint32_t triangleB = intersectionInfo.triangleB;
            const Vector3d &q0 = inputPositionsB[3 * triangleB + 0];
            const Vector3d &q1 = inputPositionsB[3 * triangleB + 1];
            const Vector3d &q2 = inputPositionsB[3 * triangleB + 2];

            intersectionInfo.points.reserve(intersectionInfo.intersection.GetPoints().size());
            for(auto &inct : intersectionInfo.intersection.GetPoints())
            {
                const SI::Element elemA = inct.GetElement0();
                const SI::Element elemB = inct.GetElement1();
                intersectionInfo.points.push_back(EvaluateSymbolicIntersection(p0, p1, p2, q0, q1, q2, elemA, elemB));
            }
        }
    });

    // Retriangulate A

    std::vector<PerTriangleOutput> triangleAToOutput(triangleCountA);
    RefineTriangles(inputPositionsA, degenerateTriangleFlagA, triangleAToPairwiseIntersections, triangleAToOutput);

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
    RefineTriangles(inputPositionsB, degenerateTriangleFlagB, triangleBToPairwiseIntersections, triangleBToOutput);

    // Final rounding

    auto RoundToFinalOutput = [](
        Span<PerTriangleOutput> triangleToOutput,
        Span<Vector3d>          inputPositions,
        std::vector<Vector3d>  &outputPositions,
        std::vector<uint32_t>  *outputFaceToInputFace)
    {
        for(uint32_t triangleIndex = 0; triangleIndex < triangleToOutput.size(); ++triangleIndex)
        {
            const PerTriangleOutput &newTriangles = triangleToOutput[triangleIndex];

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
                const Vector3d &p0 = roundedPoints[triangle[0]];
                const Vector3d &p1 = roundedPoints[triangle[1]];
                const Vector3d &p2 = roundedPoints[triangle[2]];

                if(p0 != p1 && p1 != p2 && p2 != p0) // Rounding may create new degenerate triangles. Ignored them.
                {
                    outputPositions.push_back(p0);
                    outputPositions.push_back(p1);
                    outputPositions.push_back(p2);
                    if(outputFaceToInputFace)
                    {
                        outputFaceToInputFace->push_back(triangleIndex);
                    }
                }
            }
        }
    };

    RoundToFinalOutput(triangleAToOutput, inputPositionsA, outputPositionsA, outputFaceToInputFaceA);
    RoundToFinalOutput(triangleBToOutput, inputPositionsB, outputPositionsB, outputFaceToInputFaceB);
}

RTRC_GEO_END
