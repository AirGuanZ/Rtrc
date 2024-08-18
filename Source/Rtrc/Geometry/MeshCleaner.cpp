#include <map>
#include <ranges>
#include <set>

#include <Rtrc/Core/Parallel.h>
#include <Rtrc/Core/Unreachable.h>
#include <Rtrc/Core/Math/Exact/Predicates.h>
#include <Rtrc/Geometry/MeshCleaner.h>
#include <Rtrc/Geometry/Utility.h>

RTRC_GEO_BEGIN

namespace MeshCleanDetail
{

    int FindMiddleOneInCoLinearPoints(const Vector3d (&p)[3])
    {
        int axis = -1;
        for(int i = 0; i < 3; ++i)
        {
            if(p[0][i] != p[1][i])
            {
                axis = i;
                break;
            }
        }
        assert(axis >= 0);

        const double minV = (std::min)({ p[0][axis], p[1][axis], p[2][axis] });
        const double maxV = (std::max)({ p[0][axis], p[1][axis], p[2][axis] });
        for(int i = 0; i < 3; ++i)
        {
            if(p[i][axis] != minV && p[i][axis] != maxV)
            {
                return i;
            }
        }
        Unreachable();
    }

    void RemoveDegenerateTriangles(
        const IndexedPositions<double> &rawInput,
        std::vector<Vector3d>          &outputPositions,
        std::vector<uint32_t>          &outputIndices,
        std::vector<uint32_t>          *outputFaceMap,
        std::vector<uint32_t>          *outputVertexMap,
        std::vector<uint32_t>          *outputInverseVertexMap)
    {
        outputPositions.clear();
        outputIndices.clear();
        if(outputFaceMap)
        {
            outputFaceMap->clear();
        }

        // Remove duplicates in input

        std::vector<Vector3d> positions;  std::vector<uint32_t> indices;
        MergeCoincidentVertices(rawInput, positions, indices);

        const uint32_t inputVertexCount = positions.size();
        const uint32_t inputTriangleCount = indices.size() / 3;

        std::vector<uint32_t> faceMap(inputTriangleCount);
        for(uint32_t f = 0; f < inputTriangleCount; ++f)
        {
            faceMap[f] = f;
        }

        // Compute which triangles and vertices should be kept.
        // Construct a mapping from each edge to its (non-degenerate) triangles.

        struct Edge
        {
            uint32_t v0 = 0;
            uint32_t v1 = 0;
            Edge() = default;
            Edge(uint32_t a, uint32_t b)
            {
                assert(a != b);
                if(a < b)
                {
                    v0 = a;
                    v1 = b;
                }
                else
                {
                    v0 = b;
                    v1 = a;
                }
            }
            auto operator<=>(const Edge &) const = default;
        };

        std::vector<uint8_t> shouldKeepVertex(inputVertexCount);
        std::vector<uint8_t> shouldKeepTriangle(inputTriangleCount);
        std::map<Edge, std::vector<uint32_t>> edgeToTriangles;
        
        for(uint32_t f = 0; f < inputTriangleCount; ++f)
        {
            const uint32_t v0 = indices[3 * f + 0];
            const uint32_t v1 = indices[3 * f + 1];
            const uint32_t v2 = indices[3 * f + 2];
            if(v0 == v1 || v1 == v2 || v2 == v0)
            {
                continue;
            }

            const Vector3d &p0 = positions[v0];
            const Vector3d &p1 = positions[v1];
            const Vector3d &p2 = positions[v2];
            if(AreCoLinear(p0, p1, p2))
            {
                continue;
            }

            shouldKeepVertex[v0] = true;
            shouldKeepVertex[v1] = true;
            shouldKeepVertex[v2] = true;

            shouldKeepTriangle[f] = true;

            edgeToTriangles[Edge(v0, v1)].push_back(f);
            edgeToTriangles[Edge(v1, v2)].push_back(f);
            edgeToTriangles[Edge(v2, v0)].push_back(f);
        }

        // Collect split points for each edge

        std::map<Edge, std::vector<uint32_t>> edgeToSplitPoints;

        for(uint32_t f = 0; f < inputTriangleCount; ++f)
        {
            if(shouldKeepTriangle[f])
            {
                continue;
            }

            const uint32_t v0 = indices[3 * f + 0];
            const uint32_t v1 = indices[3 * f + 1];
            const uint32_t v2 = indices[3 * f + 2];
            if(v0 == v1 || v1 == v2 || v2 == v0)
            {
                continue;
            }

            const Vector3d &p0 = positions[v0];
            const Vector3d &p1 = positions[v1];
            const Vector3d &p2 = positions[v2];
            const int middleIndex = FindMiddleOneInCoLinearPoints({ p0, p1, p2 });
            
            const uint32_t vs[3] = { v0, v1, v2 };
            const uint32_t vm = vs[middleIndex];
            const uint32_t va = vs[(middleIndex + 1) % 3];
            const uint32_t vb = vs[(middleIndex + 2) % 3];
            if(shouldKeepVertex[va] && shouldKeepVertex[vb])
            {
                edgeToTriangles[Edge(va, vb)].push_back(vm);
            }
        }

        // Split triangles

        for(auto &[edge, splitPoints] : edgeToSplitPoints)
        {
            // Sort split points from edge.v0 to edge.v1

            int axis = -1;
            for(int i = 0; i < 3; ++i)
            {
                if(positions[edge.v0][i] != positions[edge.v1][i])
                {
                    axis = i;
                    break;
                }
            }
            assert(axis >= 0);

            if(positions[edge.v0][axis] < positions[edge.v1][axis])
            {
                std::ranges::sort(
                    splitPoints,
                    [&](uint32_t a, uint32_t b) { return positions[a][axis] < positions[b][axis]; });
            }
            else
            {
                std::ranges::sort(
                    splitPoints,
                    [&](uint32_t a, uint32_t b) { return positions[a][axis] > positions[b][axis]; });
            }

            // Split triangles containing edge

            auto &toBeSplitTriangles = edgeToTriangles[edge];
            std::vector<uint32_t> newTriangles;
            for(uint32_t triangle : toBeSplitTriangles)
            {
                const uint32_t tv0 = indices[3 * triangle + 0];
                const uint32_t tv1 = indices[3 * triangle + 1];
                const uint32_t tv2 = indices[3 * triangle + 2];

                const bool reverseWindingOrder =
                    !(tv0 == edge.v0 && tv1 == edge.v1) &&
                    !(tv1 == edge.v0 && tv2 == edge.v1) &&
                    !(tv2 == edge.v0 && tv0 == edge.v1);

                const uint32_t anotherV = Edge(tv0, tv1) == edge ? tv2 :
                                          Edge(tv1, tv2) == edge ? tv0 : tv1;

                const uint32_t firstNewTriangle = indices.size() / 3;

                uint32_t lastSplitPoint = edge.v0;
                for(uint32_t sv : splitPoints)
                {
                    newTriangles.push_back(indices.size() / 3);
                    faceMap.push_back(triangle);

                    if(reverseWindingOrder)
                    {
                        indices.push_back(anotherV);
                        indices.push_back(sv);
                        indices.push_back(lastSplitPoint);
                    }
                    else
                    {
                        indices.push_back(lastSplitPoint);
                        indices.push_back(sv);
                        indices.push_back(anotherV);
                    }
                    lastSplitPoint = sv;
                }

                if(tv0 == edge.v0)
                {
                    indices[3 * triangle + 0] = lastSplitPoint;
                }
                else if(tv1 == edge.v0)
                {
                    indices[3 * triangle + 1] = lastSplitPoint;
                }
                else
                {
                    assert(tv2 == edge.v0);
                    indices[3 * triangle + 2] = lastSplitPoint;
                }

                if(auto it = edgeToTriangles.find(Edge(edge.v0, anotherV)); it != edgeToTriangles.end())
                {
                    for(uint32_t &t : it->second)
                    {
                        if(t == triangle)
                        {
                            t = firstNewTriangle;
                            break;
                        }
                    }
                }
            }
            std::ranges::copy(newTriangles, std::back_inserter(toBeSplitTriangles));
        }

        std::vector<uint32_t> remainingIndices;
        for(uint32_t f = 0; f < indices.size() / 3; ++f)
        {
            if(f >= inputTriangleCount || shouldKeepTriangle[f])
            {
                remainingIndices.push_back(indices[3 * f + 0]);
                remainingIndices.push_back(indices[3 * f + 1]);
                remainingIndices.push_back(indices[3 * f + 2]);
                if(outputFaceMap)
                {
                    (*outputFaceMap)[f] = faceMap[f];
                }
            }
        }

        MergeCoincidentVertices(IndexedPositions<double>(positions, remainingIndices), outputPositions, outputIndices);

        if(outputVertexMap)
        {
            outputVertexMap->resize(outputPositions.size());
        }

        if(outputInverseVertexMap)
        {
            outputInverseVertexMap->clear();
            outputInverseVertexMap->resize(rawInput.GetPositionCount(), UINT32_MAX);
        }

        if(outputVertexMap || outputInverseVertexMap)
        {
            std::map<Vector3d, std::set<uint32_t>> positionToInputVertices;
            for(auto &&[index, position] : std::views::enumerate(rawInput.GetPositions()))
            {
                positionToInputVertices[position].insert(index);
            }

            for(uint32_t i = 0; i < outputPositions.size(); ++i)
            {
                auto &inputVertices = positionToInputVertices[outputPositions[i]];

                if(outputVertexMap)
                {
                    (*outputVertexMap)[i] = *inputVertices.begin();
                }

                if(outputInverseVertexMap)
                {
                    for(uint32_t inputVertex : inputVertices)
                    {
                        (*outputInverseVertexMap)[inputVertex] = i;
                    }
                }
            }
        }
    }

} // namespace MeshCleanDetail

void MeshCleaner::RemoveDegenerateTriangles(Span<Vector3d> inputPositions, Span<uint32_t> inputIndices)
{
    using namespace MeshCleanDetail;

    outputPositions.clear();
    outputIndices.clear();
    outputFaceMap.clear();
    outputVertexMap.clear();
    outputInverseVertexMap.clear();

    MeshCleanDetail::RemoveDegenerateTriangles(
        IndexedPositions(inputPositions, inputIndices),
        outputPositions, outputIndices,
        trackFaceMap ? &outputFaceMap : nullptr,
        trackVertexMap ? &outputVertexMap : nullptr,
        trackInverseVertexMap ? &outputInverseVertexMap : nullptr);
}

RTRC_GEO_END
