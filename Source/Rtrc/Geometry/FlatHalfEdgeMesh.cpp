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

    InternalSetVert(a0, v3, {        });
    InternalSetVert(a1, v2, { a2     });
    InternalSetVert(a2, v0, { b1, a0 });
    InternalSetVert(b0, v2, {        });
    InternalSetVert(b1, v3, { b2     });
    InternalSetVert(b2, v1, { a1, b0 });

    InternalSetTwin(a0, b0);
    InternalSetTwin(a1, a2t);
    InternalSetTwin(a2, b1t);
    InternalSetTwin(b1, b2t);
    InternalSetTwin(b2, a1t);

    InternalSetEdge(b2, a1e, { a1 });
    InternalSetEdge(a1, a2e, { a2 });
    InternalSetEdge(a2, b1e, { b1 });
    InternalSetEdge(b1, b2e, { b2 });
}

void FlatHalfedgeMesh::SplitEdge(int h)
{
    if(const int twin = Twin(h); twin < 0)
    {
        // Before

        const int a0 = h;
        const int a1 = Succ(a0);
        const int a2 = Succ(a1);

        const int a1t = Twin(a1);

        const int v1 = Vert(a1);
        const int v2 = Vert(a2);

        const int e1 = Edge(a1);

        // New

        const int v3 = V_;
        const int e3 = E_;
        const int e4 = e3 + 1;
        const int b0 = H_;
        const int b1 = b0 + 1;
        const int b2 = b1 + 1;

        H_ += 3;
        V_ += 1;
        E_ += 2;
        F_ += 1;

        halfedgeToHead_.resize(H_, -1);
        halfedgeToTwin_.resize(H_, -1);
        halfedgeToEdge_.resize(H_, -1);
        vertToHalfedge_.resize(V_, -1);
        edgeToHalfedge_.resize(E_, -1);

        InternalSetTwin(a1, b2);
        InternalSetTwin(b0, -1);
        InternalSetTwin(b1, a1t);

        InternalSetVert(a1, v3, {    });
        InternalSetVert(b0, v3, { -1 });
        InternalSetVert(b1, v1, { a1 });
        InternalSetVert(b2, v2, {    });

        InternalSetEdge(a1, e3, { -1 });
        InternalSetEdge(b0, e4, { -1 });
        InternalSetEdge(b1, e1, { a1 });
        InternalSetEdge(b2, e3, {    });
    }
    else
    {
        // Before

        const int a0 = h;
        const int a1 = Succ(a0);
        const int a2 = Succ(a1);

        const int b0 = twin;
        const int b1 = Succ(b0);
        const int b2 = Succ(b1);

        const int a1t = Twin(a1);
        const int b1t = Twin(b1);

        const int v0 = Vert(a0);
        const int v1 = Vert(b2);
        const int v2 = Vert(a1);
        const int v3 = Vert(a2);

        const int e0 = Edge(b1);
        const int e2 = Edge(a1);
        const int e4 = Edge(a0);

        // New

        const int c0 = H_ + 0;
        const int c1 = H_ + 1;
        const int c2 = H_ + 2;
        const int d0 = H_ + 3;
        const int d1 = H_ + 4;
        const int d2 = H_ + 5;

        const int v4 = V_;

        const int e5 = E_ + 0;
        const int e6 = E_ + 1;
        const int e7 = E_ + 2;

        H_ += 6;
        V_ += 1;
        E_ += 3;
        F_ += 2;

        halfedgeToHead_.resize(H_, -1);
        halfedgeToTwin_.resize(H_, -1);
        halfedgeToEdge_.resize(H_, -1);
        vertToHalfedge_.resize(V_, -1);
        edgeToHalfedge_.resize(E_, -1);

        InternalSetTwin(a0, d1);
        InternalSetTwin(a1, c0);
        InternalSetTwin(b0, c1);
        InternalSetTwin(b1, d0);
        InternalSetTwin(c2, a1t);
        InternalSetTwin(d2, b1t);

        InternalSetVert(a1, v4, { -1 });
        InternalSetVert(b1, v4, {    });
        InternalSetVert(c0, v3, {    });
        InternalSetVert(c1, v4, {    });
        InternalSetVert(c2, v2, { a1 });
        InternalSetVert(d0, v1, {    });
        InternalSetVert(d1, v4, {    });
        InternalSetVert(d2, v0, { b1 });

        InternalSetEdge(a1, e6, { -1 });
        InternalSetEdge(b0, e5, { -1 });
        InternalSetEdge(b1, e7, { -1 });
        InternalSetEdge(c0, e6, {    });
        InternalSetEdge(c1, e5, {    });
        InternalSetEdge(c2, e2, { a1 });
        InternalSetEdge(d0, e7, {    });
        InternalSetEdge(d1, e4, { b0 });
        InternalSetEdge(d2, e0, { b1 });
    }
}

void FlatHalfedgeMesh::SplitFace(int f)
{
    // Before

    const int a0 = 3 * f + 0;
    const int a1 = 3 * f + 1;
    const int a2 = 3 * f + 2;

    const int a1t = Twin(a1);
    const int a2t = Twin(a2);

    const int v0 = Vert(a0);
    const int v1 = Vert(a1);
    const int v2 = Vert(a2);

    const int e1 = Edge(a1);
    const int e2 = Edge(a2);

    // New

    const int b0 = H_ + 0;
    const int b1 = H_ + 1;
    const int b2 = H_ + 2;
    const int c0 = H_ + 3;
    const int c1 = H_ + 4;
    const int c2 = H_ + 5;

    const int e3 = E_ + 0;
    const int e4 = E_ + 1;
    const int e5 = E_ + 2;

    const int v3 = V_;

    H_ += 6;
    E_ += 3;
    V_ += 1;
    F_ += 2;

    halfedgeToHead_.resize(H_, -1);
    halfedgeToTwin_.resize(H_, -1);
    halfedgeToEdge_.resize(H_, -1);
    vertToHalfedge_.resize(V_, -1);
    edgeToHalfedge_.resize(E_, -1);

    InternalSetTwin(a1, b2);
    InternalSetTwin(a2, c1);
    InternalSetTwin(b0, a1t);
    InternalSetTwin(b1, c2);
    InternalSetTwin(c0, a2t);

    InternalSetVert(a2, v3, { -1 });
    InternalSetVert(b0, v1, { a1 });
    InternalSetVert(b1, v2, {    });
    InternalSetVert(b2, v3, {    });
    InternalSetVert(c0, v2, { a2 });
    InternalSetVert(c1, v0, {    });
    InternalSetVert(c2, v3, {    });

    InternalSetEdge(a1, e4, { -1 });
    InternalSetEdge(a2, e3, { -1 });
    InternalSetEdge(b0, e1, { a1 });
    InternalSetEdge(b1, e5, { -1 });
    InternalSetEdge(b2, e4, {    });
    InternalSetEdge(c0, e2, { a2 });
    InternalSetEdge(c1, e3, {    });
    InternalSetEdge(c2, e5, {    });
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
        if(VertToHalfedge(v) < 0 || Vert(VertToHalfedge(v)) != v)
        {
            return false;
        }
    }

    for(int e = 0; e < E(); ++e)
    {
        if(EdgeToHalfedge(e) < 0 || Edge(EdgeToHalfedge(e)) != e)
        {
            return false;
        }
    }

    return true;
}

void FlatHalfedgeMesh::InternalSetEdge(int newH, int e, Span<int> oldHs)
{
    // By setting oldHs = { -1 }, the caller must ensure that edgeToHalfedge[e] has not been initialized
    // and will always be set to newH in this call.
    assert(!(oldHs.size() > 1 && oldHs[0] == -1));
    assert(!(!oldHs.IsEmpty() && oldHs[0] == -1 && edgeToHalfedge_[e] != -1));
    halfedgeToEdge_[newH] = e;
    for(int oldH : oldHs)
    {
        if(edgeToHalfedge_[e] == oldH)
        {
            edgeToHalfedge_[e] = newH;
            break;
        }
    }
}

void FlatHalfedgeMesh::InternalSetVert(int newH, int v, Span<int> oldHs)
{
    assert(!(oldHs.size() > 1 && oldHs[0] == -1));
    assert(!(!oldHs.IsEmpty() && oldHs[0] == -1 && vertToHalfedge_[v] != -1));
    halfedgeToHead_[newH] = v;
    for(int oldH : oldHs)
    {
        if(vertToHalfedge_[v] == oldH)
        {
            vertToHalfedge_[v] = newH;
            break;
        }
    }
}

void FlatHalfedgeMesh::InternalSetTwin(int h0, int h1)
{
    if(h0 >= 0) { halfedgeToTwin_[h0] = h1; }
    if(h1 >= 0) { halfedgeToTwin_[h1] = h0; }
}

RTRC_END
