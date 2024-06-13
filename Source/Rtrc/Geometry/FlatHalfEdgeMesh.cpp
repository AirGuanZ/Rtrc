#include <ankerl/unordered_dense.h>

#include <Rtrc/Core/Hash.h>
#include <Rtrc/Geometry/FlatHalfedgeMesh.h>

RTRC_BEGIN

FlatHalfedgeMesh FlatHalfedgeMesh::Build(Span<uint32_t> indices, BuildOptions options)
{
    struct EdgeRecord
    {
        int h0;
        int h1;
    };
    std::vector<EdgeRecord> edgeRecords;
    ankerl::unordered_dense::map<std::pair<int, int>, int, HashOperator<>> vertsToEdge;

    int maxV = (std::numeric_limits<int>::min)();
    std::vector<int> heads, twins, edges;

    assert(indices.size() % 3 == 0);
    const unsigned F = indices.size() / 3;
    for(unsigned faceIndex = 0; faceIndex < F; ++faceIndex)
    {
        auto HandleEdge = [&](int head, int tail)
        {
            maxV = (std::max)(maxV, head);

            const int h = static_cast<int>(heads.size());
            heads.push_back(head);

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
                    if(options.Contains(ThrowOnInvalidInput))
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

    // Check for non-manifold vertices.
    // A non-manifold vertex has more than one incident boundary halfedge.

    std::vector<bool> vertexToBoundaryIncidentHalfedges(maxV + 1, false);
    for(int h = 0; h < static_cast<int>(heads.size()); ++h)
    {
        if(twins[h] >= 0)
        {
            continue;
        }
        const int vert = heads[h];
        if(vertexToBoundaryIncidentHalfedges[vert])
        {
            if(options.Contains(ThrowOnInvalidInput))
            {
                throw Exception(fmt::format("Non-manifold vertex encountered. Vertex is {}", vert));
            }
            return {};
        }
        vertexToBoundaryIncidentHalfedges[vert] = true;
    }

    // Fill final mesh

    FlatHalfedgeMesh mesh;
    mesh.H_ = static_cast<int>(heads.size());
    mesh.V_ = maxV + 1;
    mesh.E_ = static_cast<int>(edgeRecords.size());
    mesh.F_ = F;
    mesh.halfedgeToTwin_ = std::move(twins);
    mesh.halfedgeToEdge_ = std::move(edges);
    mesh.halfedgeToHead_ = std::move(heads);

    mesh.vertToHalfedge_.resize(mesh.V_, -1);
    for(int h = 0; h < mesh.H_; ++h)
    {
        const int v = mesh.Vert(h);
        if(mesh.vertToHalfedge_[v] < 0 || mesh.halfedgeToTwin_[h] < 0)
        {
            mesh.vertToHalfedge_[v] = h;
        }
    }

    // Check vertex references

    for(int v = 0; v < mesh.V_; ++v)
    {
        if(mesh.vertToHalfedge_[v] < 0)
        {
            if(options.Contains(ThrowOnInvalidInput))
            {
                throw Exception(fmt::format("Vertex {} is not contained by any face", v));
            }
            return {};
        }
    }

    mesh.edgeToHalfedge_.resize(mesh.E_);
    for(int e = 0; e < mesh.E_; ++e)
    {
        const EdgeRecord &record = edgeRecords[e];
        mesh.edgeToHalfedge_[e] = record.h0;
    }

    return mesh;
}

void FlatHalfedgeMesh::FlipEdge(int e)
{
    const int a0 = edgeToHalfedge_[e];
    const int b0 = Twin(a0);
    assert(b0 >= 0);

    const int a1  = Succ(a0);
    const int a2  = Succ(a1);
    const int a1t = Twin(a1);
    const int a2t = Twin(a2);
    const int a1e = Edge(a1);
    const int a2e = Edge(a2);

    const int b1  = Succ(b0);
    const int b2  = Succ(b1);
    const int b1t = Twin(b1);
    const int b2t = Twin(b2);
    const int b1e = Edge(b1);
    const int b2e = Edge(b2);

    const int v0 = Vert(a0);
    const int v1 = Vert(a1);
    const int v2 = Vert(a2);
    const int v3 = Vert(b2);

    auto SetVert = [&](int vert, int oldH0, int oldH1, int newH)
    {
        halfedgeToHead_[newH] = vert;
        if(vertToHalfedge_[vert] == oldH0 || vertToHalfedge_[vert] == oldH1)
        {
            vertToHalfedge_[vert] = newH;
        }
    };
    SetVert(v3, -1, -1, a0);
    SetVert(v2, a2, -1, a1);
    SetVert(v0, b1, a0, a2);
    SetVert(v2, -1, -1, b0);
    SetVert(v3, b2, -1, b1);
    SetVert(v1, a1, b0, b2);

    auto SetTwin = [&](int h0, int h1)
    {
        if(h0 >= 0) { halfedgeToTwin_[h0] = h1; }
        if(h1 >= 0) { halfedgeToTwin_[h1] = h0; }
    };
    SetTwin(a0, b0);
    SetTwin(a1, a2t);
    SetTwin(a2, b1t);
    SetTwin(b1, b2t);
    SetTwin(b2, a1t);

    auto SetEdge = [&](int edge, int oldH, int newH)
    {
        halfedgeToEdge_[newH] = edge;
        if(edgeToHalfedge_[edge] == oldH)
        {
            edgeToHalfedge_[edge] = newH;
        }
    };
    SetEdge(a1e, a1, b2);
    SetEdge(a2e, a2, a1);
    SetEdge(b1e, b1, a2);
    SetEdge(b2e, b2, b1);
}

bool FlatHalfedgeMesh::CheckSanity() const
{
    for(int h = 0; h < H(); ++h)
    {
        const int twin = Twin(h);
        if(twin >= 0 && Twin(twin) != h)
        {
            return false;
        }
    }

    for(int v = 0; v < V(); ++v)
    {
        if(Vert(VertToHalfedge(v)) != v)
        {
            return false;
        }
    }

    for(int e = 0; e < E(); ++e)
    {
        if(Edge(EdgeToHalfedge(e)) != e)
        {
            return false;
        }
    }

    return true;
}

RTRC_END
