#if _MSC_VER
#pragma warning(push)
#pragma warning(disable:4819)
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <igl/lscm.h>
#if _MSC_VER
#pragma warning(pop)
#endif

#include "Mesh.h"

InputMesh InputMesh::Load(const std::string& filename)
{
    RTRC_LOG_INFO_SCOPE("Load mesh from {}", filename);

    MeshData meshData;

    {
        RTRC_LOG_INFO_SCOPE("Load obj file");
        meshData = MeshData::LoadFromObjFile(filename, false, false);
        LogInfo("#Vertex = {}", meshData.positionData.size());
        LogInfo("#Triangle = {}", meshData.indexData.size() / 3);
    }

    {
        RTRC_LOG_INFO_SCOPE("Merge identical vertices");
        meshData.MergeIdenticalPositions();
        LogInfo("#Vertex = {}", meshData.positionData.size());
        LogInfo("#Triangle = {}", meshData.indexData.size() / 3);
    }

    Vector3f gridLower((std::numeric_limits<float>::max)()), gridUpper(std::numeric_limits<float>::lowest());
    Vector3f lower(std::numeric_limits<float>::max()), upper(std::numeric_limits<float>::lowest());

    LogInfo("Find mesh boundary");
    {
        for(auto &p : meshData.positionData)
        {
            if(std::abs(p.y) < 1e-5f)
            {
                gridLower = Min(gridLower, p);
                gridUpper = Max(gridUpper, p);
            }
            lower = Min(lower, p);
            upper = Max(upper, p);
        }
    }

    auto ComputeBoundaryUV = [&](const Vector3f &p, Vector2f &uv)
    {
        if(std::abs(p.y) >= 1e-5f)
        {
            return false;
        }
        auto Eq = [&](float x, float y)
        {
            return std::abs(x - y) < 1e-4f;
        };
        const bool xl = Eq(p.x, gridLower.x);
        const bool xu = Eq(p.x, gridUpper.x);
        const bool zl = Eq(p.z, gridLower.z);
        const bool zu = Eq(p.z, gridUpper.z);
        float u, v;
        if(xl)
        {
            u = 0;
        }
        else if(xu)
        {
            u = 1;
        }
        else
        {
            u = std::clamp((p.x - gridLower.x) / (gridUpper.x - gridLower.x), 0.0f, 1.0f);
        }
        if(zl)
        {
            v = 0;
        }
        else if(zu)
        {
            v = 1;
        }
        else
        {
            v = std::clamp((p.z - gridLower.z) / (gridUpper.z - gridLower.z), 0.0f, 1.0f);
        }
        uv = { u, v };
        return xl || xu || zl || zu;
    };

    Eigen::MatrixX3d V;
    Eigen::MatrixX3i F;
    Eigen::VectorXi b;
    Eigen::MatrixX2d bc;

    LogInfo("Construct mesh matrices");
    {
        V.resize(meshData.positionData.size(), 3);
        for(unsigned i = 0; i < meshData.positionData.size(); ++i)
        {
            auto &p = meshData.positionData[i];
            V.row(i) = Eigen::Vector3d(p.x, p.y, p.z);
        }

        F.resize(meshData.indexData.size() / 3, 3);
        for(unsigned i = 0; i < meshData.indexData.size() / 3; ++i)
        {
            auto f = &meshData.indexData[3 * i];
            F.row(i) = Eigen::Vector3i(f[0], f[1], f[2]);
        }

        struct BoundaryVertex
        {
            unsigned index;
            Vector2f uv;
        };
        std::vector<BoundaryVertex> boundaryVertices;
        for(unsigned i = 0; i < meshData.positionData.size(); ++i)
        {
            Vector2f uv;
            if(ComputeBoundaryUV(meshData.positionData[i], uv))
            {
                boundaryVertices.push_back({ i, uv });
            }
        }

        b.resize(boundaryVertices.size(), 1);
        bc.resize(boundaryVertices.size(), 2);
        for(unsigned i = 0; i < boundaryVertices.size(); ++i)
        {
            auto &v = boundaryVertices[i];
            b[i] = v.index;
            bc.row(i) = Eigen::Vector2d(v.uv.x, v.uv.y);
        }
    }

    Eigen::MatrixX2d UV;
    
    LogInfo("LSCM");
    {
        if(!igl::lscm(V, F, b, bc, UV))
        {
            throw Exception("Fail to run lscm on input mesh");
        }
    }

    LogInfo("Build final mesh");

    InputMesh ret;
    ret.positions = meshData.positionData;
    ret.indices = meshData.indexData;
    ret.parameterSpacePositions.resize(meshData.positionData.size());
    ret.gridLower = gridLower.xz();
    ret.gridUpper = gridUpper.xz();
    ret.lower = lower;
    ret.upper = upper;
    for(unsigned i = 0; i < meshData.positionData.size(); ++i)
    {
        auto row = UV.row(i);
        ret.parameterSpacePositions[i].x = static_cast<float>(row[0]);
        ret.parameterSpacePositions[i].y = static_cast<float>(row[1]);
    }

    return ret;
}

void InputMesh::DetectSharpFeatures(float angleThreshold)
{
    RTRC_LOG_INFO_SCOPE("Detect sharp features");

    sharpFeatures = {};

    // Sharp edges

    auto &vertexImportance = sharpFeatures.vertexImportance;
    auto &sharpEdges = sharpFeatures.edges;
    {
        RTRC_LOG_INFO_SCOPE("Collect sharp edges");

        vertexImportance.resize(positions.size(), 0);

        struct EdgeRecord
        {
            int f0 = -1;
            int f1 = -1;
        };

        const float angleDotThreshold = std::cos(angleThreshold);

        std::map<std::pair<int, int>, int> e2v;

        for(unsigned i = 0; i < indices.size(); i += 3)
        {
            for(unsigned j = 0; j < 3; ++j)
            {
                int ia = indices[i + j];
                int ib = indices[i + (j + 1) % 3];
                const int ic = indices[i + (j + 2) % 3];
                if(ia > ib)
                {
                    std::swap(ia, ib);
                }

                if(auto it = e2v.find({ ia, ib }); it != e2v.end())
                {
                    const int id = it->second;
                    const Vector3f &pa = positions[ia];
                    const Vector3f &pb = positions[ib];
                    const Vector3f &pc = positions[ic];
                    const Vector3f &pd = positions[id];
                    const Vector3f nor0 = +Normalize(Cross(pc - pb, pb - pa));
                    const Vector3f nor1 = -Normalize(Cross(pd - pb, pb - pa));

                    const float dot = Dot(nor0, nor1);
                    if(dot <= angleDotThreshold)
                    {
                        sharpEdges.push_back({ ia, ib });
                    }
                    e2v.erase(it);

                    const float importance = 0.5f * (1 - dot);
                    vertexImportance[ia] = (std::max)(vertexImportance[ia], importance);
                    vertexImportance[ib] = (std::max)(vertexImportance[ib], importance);
                }
                else
                {
                    e2v.insert({ { ia, ib }, ic });
                }
            }
        }

        LogInfo("#SharpEdges = {}", sharpEdges.size());
        LogInfo("#BoundaryEdges = {}", e2v.size());
    }

    // Vertex -> sharp edge list

    std::vector<std::vector<int>> vertexToSharpEdgeList(positions.size());

    {
        RTRC_LOG_INFO_SCOPE("Build vertex -> sharp edge list");

        for(auto &&[i, edge] : Enumerate(sharpEdges))
        {
            vertexToSharpEdgeList[edge.v0].push_back(i);
            vertexToSharpEdgeList[edge.v1].push_back(i);
        }

        for(auto &sharpEdgeList : vertexToSharpEdgeList)
        {
            std::ranges::sort(sharpEdgeList);
        }
    }

    // Detect corners

    {
        RTRC_LOG_INFO_SCOPE("Detect corners");

        for(unsigned v = 0; v < positions.size(); ++v)
        {
            if(vertexToSharpEdgeList[v].size() >= 3)
            {
                sharpFeatures.corners.push_back(v);
            }
        }
    }

    // Detect segments

    {
        RTRC_LOG_INFO_SCOPE("Detect segments");

        unsigned nextSharpEdgeIndex = 0;
        std::vector isSharpEdgeProcessed(sharpEdges.size(), false);
        while(true)
        {
            if(nextSharpEdgeIndex >= sharpEdges.size())
            {
                break;
            }

            if(isSharpEdgeProcessed[nextSharpEdgeIndex])
            {
                ++nextSharpEdgeIndex;
                continue;
            }

            const int eCenter = nextSharpEdgeIndex;
            isSharpEdgeProcessed[nextSharpEdgeIndex] = true;
            RTRC_SCOPE_EXIT{ ++nextSharpEdgeIndex; };

            int v0 = sharpEdges[nextSharpEdgeIndex].v0;
            int v1 = sharpEdges[nextSharpEdgeIndex].v1;
            std::vector<int> edgesAlongV0, edgesAlongV1;

            while(true)
            {
                if(vertexToSharpEdgeList[v0].size() == 2)
                {
                    const int e0 = vertexToSharpEdgeList[v0][0];
                    const int e1 = vertexToSharpEdgeList[v0][1];
                    const int e = isSharpEdgeProcessed[e0] ? e1 : e0;
                    if(isSharpEdgeProcessed[e])
                    {
                        break;
                    }
                    isSharpEdgeProcessed[e] = true;
                    edgesAlongV0.push_back(e);
                    const int ev0 = sharpEdges[e].v0;
                    const int ev1 = sharpEdges[e].v1;
                    assert(ev0 == v0 || ev1 == v0);
                    v0 = ev0 != v0 ? ev0 : ev1;
                    continue;
                }

                if(vertexToSharpEdgeList[v1].size() == 2)
                {
                    const int e0 = vertexToSharpEdgeList[v1][0];
                    const int e1 = vertexToSharpEdgeList[v1][1];
                    const int e = isSharpEdgeProcessed[e0] ? e1 : e0;
                    if(isSharpEdgeProcessed[e])
                    {
                        break;
                    }
                    isSharpEdgeProcessed[e] = true;
                    edgesAlongV1.push_back(e);
                    const int ev0 = sharpEdges[e].v0;
                    const int ev1 = sharpEdges[e].v1;
                    assert(ev0 == v1 || ev1 == v1);
                    v1 = ev0 != v1 ? ev0 : ev1;
                    continue;
                }

                break;
            }

            auto &newSegment = sharpFeatures.segments.emplace_back();
            newSegment.edges.reserve(edgesAlongV0.size() + 1 + edgesAlongV1.size());
            for(auto it = edgesAlongV0.rbegin(); it != edgesAlongV0.rend(); ++it)
            {
                newSegment.edges.push_back(*it);
            }
            newSegment.edges.push_back(eCenter);
            for(int e : edgesAlongV1)
            {
                newSegment.edges.push_back(e);
            }
        }
    }

    {
        RTRC_LOG_INFO_SCOPE("Fill sharp edge indices");

        sharpEdgeIndices.clear();
        sharpEdgeIndices.reserve(sharpEdges.size());
        for(auto &e : sharpEdges)
        {
            sharpEdgeIndices.push_back(e.v0);
            sharpEdgeIndices.push_back(e.v1);
        }
    }
}

void InputMesh::Upload(DeviceRef device)
{
    positionBuffer = device->CreateAndUploadStructuredBuffer(Span<Vector3f>{ positions });
    parameterSpacePositionBuffer = device->CreateAndUploadStructuredBuffer(Span<Vector2f>{ parameterSpacePositions });
    indexBuffer = device->CreateAndUploadStructuredBuffer(RHI::BufferUsage::IndexBuffer, Span<uint32_t>{ indices });
    if(!sharpEdgeIndices.empty())
    {
        sharpEdgeIndexBuffer =
            device->CreateAndUploadStructuredBuffer(RHI::BufferUsage::IndexBuffer, Span<uint32_t>(sharpEdgeIndices));
    }
    else
    {
        sharpEdgeIndexBuffer = {};
    }
}
