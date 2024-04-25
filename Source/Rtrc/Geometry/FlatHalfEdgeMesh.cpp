#include <ankerl/unordered_dense.h>

#include <Rtrc/Core/Hash.h>
#include <Rtrc/Geometry/FlatHalfEdgeMesh.h>

RTRC_BEGIN

FlatHalfEdgeMesh FlatHalfEdgeMesh::Build(Span<int32_t> indices, BuildOptions options)
{
    assert(indices.IsEmpty() || indices.last() < 0);

    struct EdgeRecord
    {
        int h0;
        int h1;
    };
    std::vector<EdgeRecord> edgeRecords;
    ankerl::unordered_dense::map<std::pair<int, int>, int, HashOperator<>> vertsToEdge;

    int maxV = (std::numeric_limits<int>::min)();
    std::vector<int> heads, faces, twins, edges;

    assert(indices.size() % 3 == 0);
    const unsigned F = indices.size() / 3;
    for(unsigned faceIndex = 0; faceIndex < F; ++faceIndex)
    {
        auto HandleEdge = [&](int head, int tail)
        {
            maxV = (std::max)(maxV, head);

            const int h = static_cast<int>(heads.size());
            heads.push_back(head);
            faces.push_back(faceIndex);

            int edge;
            const int vMin = (std::min)(head, tail);
            const int vMax = (std::max)(head, tail);
            if(auto it = vertsToEdge.find({ vMin, vMax }); it != vertsToEdge.end())
            {
                edge = it->second;
                EdgeRecord &record = edgeRecords[edge];
                assert(record.h0 >= 0);
                if(record.h1 >= 0)
                {
                    if(options.Contains(ThrowOnNonManifoldInput))
                    {
                        throw Exception(fmt::format(
                            "Non-manifold edge encountered. Vertices in edge are {} and {}", head, tail));
                    }
                    return false;
                }
                record.h1 = h;
                twins[record.h0] = h;
                twins.push_back(record.h0);
            }
            else
            {
                edge = static_cast<int>(edgeRecords.size());
                twins.push_back(-1);
                edgeRecords.emplace_back() = EdgeRecord{ h, -1 };
                vertsToEdge.try_emplace({ vMin, vMax }, edge);
            }
            edges.push_back(edge);

            return true;
        };

        const int v0 = indices[3 * faceIndex + 0];
        const int v1 = indices[3 * faceIndex + 1];
        const int v2 = indices[3 * faceIndex + 2];
        if(!HandleEdge(v0, v1) || !HandleEdge(v1, v2) || !HandleEdge(v2, v0))
        {
            return {};
        }
    }

    FlatHalfEdgeMesh mesh;
    mesh.H_ = static_cast<int>(heads.size());
    mesh.V_ = maxV + 1;
    mesh.E_ = static_cast<int>(edgeRecords.size());
    mesh.F_ = F;
    mesh.halfEdgeToTwin_ = std::move(twins);
    mesh.halfEdgeToEdge_ = std::move(edges);
    mesh.halfEdgeToHead_ = std::move(heads);
    mesh.halfEdgeToFace_ = std::move(faces);

    mesh.vertToHalfEdge_.resize(mesh.V_, -1);
    for(int h = 0; h < mesh.H_; ++h)
    {
        const int v = mesh.Head(h);
        if(mesh.vertToHalfEdge_[v] < 0)
        {
            mesh.vertToHalfEdge_[v] = h;
        }
    }

    mesh.edgeToHalfEdge_.resize(mesh.E_);
    for(int e = 0; e < mesh.E_; ++e)
    {
        const EdgeRecord &record = edgeRecords[e];
        mesh.edgeToHalfEdge_[e] = record.h0;
    }

    mesh.faceToHalfEdge_.resize(mesh.F_);
    for(int h = 0; h < mesh.H_; ++h)
    {
        const int f = mesh.Face(h);
        if(mesh.faceToHalfEdge_[f] < 0)
        {
            mesh.faceToHalfEdge_[f] = h;
        }
    }

    return mesh;
}

RTRC_END
