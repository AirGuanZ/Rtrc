#include <ankerl/unordered_dense.h>

#include <Rtrc/Geometry/HalfedgeMesh.h>

RTRC_GEO_BEGIN

HalfedgeMesh HalfedgeMesh::Build(Span<uint32_t> indices, BuildOptions options)
{
    struct EdgeRecord
    {
        std::vector<int> halfedges;
        bool isNonManifold = false;
    };
    std::map<std::pair<int, int>, EdgeRecord> vertPairToEdge;
    
    int vertexCount = 0;
    std::vector<int> heads;
    heads.reserve(indices.size());

    assert(indices.size() % 3 == 0);
    const unsigned faceCount = indices.size() / 3;
    for(unsigned faceIndex = 0; faceIndex < faceCount; ++faceIndex)
    {
        auto HandleEdge = [&](int head, int tail)
        {
            vertexCount = (std::max)(vertexCount, head + 1);

            const int h = static_cast<int>(heads.size());
            heads.push_back(head);

            bool meetNonManifoldEdge = false;
            if(auto it = vertPairToEdge.find({ head, tail }); it != vertPairToEdge.end())
            {
                it->second.halfedges.push_back(h);
                it->second.isNonManifold = true;
                meetNonManifoldEdge = true;
            }
            else if(auto jt = vertPairToEdge.find({ tail, head }); jt != vertPairToEdge.end())
            {
                auto &record = jt->second;
                if(record.halfedges.size() == 1)
                {
                    assert(!record.isNonManifold);
                    record.halfedges.push_back(h);
                }
                else
                {
                    record.halfedges.push_back(h);
                    record.isNonManifold = true;
                    meetNonManifoldEdge = true;
                }
            }
            else
            {
                vertPairToEdge.insert({ { head, tail }, { { h }, false } });
            }

            if(meetNonManifoldEdge)
            {
                if(!options.Contains(SplitNonManifoldEdge))
                {
                    if(options.Contains(ThrowOnInvalidInput))
                    {
                        throw Exception(fmt::format(
                            "Non-manifold edge encountered. Vertices in edge are {} and {}", head, tail));
                    }
                    return false;
                }
            }
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

    std::vector<int> twins(heads.size(), NullID);
    std::vector<int> edges(heads.size(), NullID);
    int edgeCount = 0;
    bool hasNonManifoldEdge = false;

    for(auto &edgeRecord : std::views::values(vertPairToEdge))
    {
        if(edgeRecord.isNonManifold)
        {
            assert(options.Contains(SplitNonManifoldEdge));
            assert(edgeRecord.halfedges.size() >= 2);
            for(int h : edgeRecord.halfedges)
            {
                edges[h] = edgeCount++;
            }
            hasNonManifoldEdge = true;
        }
        else
        {
            assert(edgeRecord.halfedges.size() == 1 || edgeRecord.halfedges.size() == 2);
            const int edge = edgeCount++;
            edges[edgeRecord.halfedges[0]] = edge;
            if(edgeRecord.halfedges.size() == 2)
            {
                edges[edgeRecord.halfedges[1]] = edge;
                twins[edgeRecord.halfedges[0]] = edgeRecord.halfedges[1];
                twins[edgeRecord.halfedges[1]] = edgeRecord.halfedges[0];
            }
        }
    }

    // Check non-manifold vertices.

    const int originalVertexCount = vertexCount;

    std::vector<bool> isHalfedgeProcessed(heads.size(), false);

    // Collect all halfedges in the the bundle of boundary halfedge h, and mark them as processed.
    auto CollectBoundaryHalfedgeBundle = [&](int h, std::vector<int> &halfedgeBundle)
    {
        assert(twins[h] == NullID);
        do
        {
            assert(!isHalfedgeProcessed[h]);
            isHalfedgeProcessed[h] = true;
            halfedgeBundle.push_back(h);
            h = twins[StaticPrev(h)];
        }
        while(h != NullID);
    };

    // Collect all halfedges in the same closed bundle of h, and mark all of them as processed.
    auto CollectClosedHalfedgeBundle = [&](int h, std::vector<int> &bundleHalfedges)
    {
        const int h0 = h;
        do
        {
            assert(!isHalfedgeProcessed[h]);
            isHalfedgeProcessed[h] = true;
            bundleHalfedges.push_back(h);
            h = twins[StaticPrev(h)];
            assert(h != NullID);
        }
        while(h != h0);
    };

    std::vector<bool> isVertexOccupied(vertexCount, false);

    // Splitting non-manifold vertices can result in the creation of new vertices. For each newly created vertex v,
    // vertexToOriginalVertex[v - originalVertexCount] records the vertex from which v was originally derived.
    std::vector<int> vertexToOriginalVertex;

    // For a given halfedge bundle B, if the vertex v associated with B hasn't been occupied by any other bundle,
    // then mark v as occupied by B. If v is already occupied, it must be a non-manifold vertex.
    // In such cases, if 'SplitNonManifoldVertex' is enabled, create a new vertex for the halfedges in bundle B.
    auto ProcessHalfedgeBundle = [&](const std::vector<int> &halfedgeBundle)
    {
        assert(!halfedgeBundle.empty());
        const int v = heads[halfedgeBundle[0]];
        if(!isVertexOccupied[v])
        {
            isVertexOccupied[v] = true;
            return true;
        }

        // This vertex has more than one halfedge bundle and thus is a non-manifold vertex
        if(!options.Contains(SplitNonManifoldVertex))
        {
            if(options.Contains(ThrowOnInvalidInput))
            {
                throw Exception(fmt::format("Non-manifold vertex encountered. Vertex id is {}", v));
            }
            return false;
        }

        const int newV = vertexCount++;
        for(int halfedgeInBundle : halfedgeBundle)
        {
            heads[halfedgeInBundle] = newV;
        }
        vertexToOriginalVertex.push_back(v);
        return true;
    };

    std::vector<int> tempHalfedgeBundle;
    for(int h = 0; h < static_cast<int>(heads.size()); ++h)
    {
        if(twins[h] == NullID)
        {
            assert(!isHalfedgeProcessed[h]);
            tempHalfedgeBundle.clear();
            CollectBoundaryHalfedgeBundle(h, tempHalfedgeBundle);
            if(!ProcessHalfedgeBundle(tempHalfedgeBundle))
            {
                return {};
            }
        }
    }

    for(int h = 0; h < static_cast<int>(heads.size()); ++h)
    {
        // All boundary bundles are already handled previously. Any halfedge that hasn't been processed must be
        // contained in a closed bundle.
        if(!isHalfedgeProcessed[h])
        {
            assert(twins[h] != NullID);
            tempHalfedgeBundle.clear();
            CollectClosedHalfedgeBundle(h, tempHalfedgeBundle);
            if(!ProcessHalfedgeBundle(tempHalfedgeBundle))
            {
                return {};
            }
        }
    }

    // Fill final mesh

    HalfedgeMesh mesh;
    mesh.H_                  = static_cast<int>(heads.size());
    mesh.V_                  = vertexCount;
    mesh.E_                  = edgeCount;
    mesh.F_                  = static_cast<int>(faceCount);
    mesh.halfedgeToTwin_     = std::move(twins);
    mesh.halfedgeToEdge_     = std::move(edges);
    mesh.halfedgeToHead_     = std::move(heads);
    mesh.isInputManifold_    = !hasNonManifoldEdge && vertexToOriginalVertex.empty();
    mesh.originalVertCount_  = originalVertexCount;
    mesh.vertToOriginalVert_ = std::move(vertexToOriginalVertex);

    mesh.vertToHalfedge_.resize(mesh.V_, NullID);
    for(int h = 0; h < mesh.H_; ++h)
    {
        const int v = mesh.Vert(h);
        if(mesh.vertToHalfedge_[v] == NullID || mesh.halfedgeToTwin_[h] == NullID)
        {
            mesh.vertToHalfedge_[v] = h;
        }
    }

    // Check vertex references

    for(int v = 0; v < mesh.V_; ++v)
    {
        if(mesh.vertToHalfedge_[v] == NullID)
        {
            if(options.Contains(ThrowOnInvalidInput))
            {
                throw Exception(fmt::format("Vertex {} is not contained by any face", v));
            }
            return {};
        }
    }

    mesh.edgeToHalfedge_.resize(mesh.E_);
    for(int h = 0; h < mesh.H_; ++h)
    {
        const int e = mesh.halfedgeToEdge_[h];
        mesh.edgeToHalfedge_[e] = h;
    }

    return mesh;
}

void HalfedgeMesh::FlipEdge(int e)
{
    const int a0 = edgeToHalfedge_[e];
    const int b0 = Twin(a0);
    assert(b0 != NullID);

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

void HalfedgeMesh::SplitEdge(int h)
{
    if(const int twin = Twin(h); twin == NullID)
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

        halfedgeToHead_.resize(H_, NullID);
        halfedgeToTwin_.resize(H_, NullID);
        halfedgeToEdge_.resize(H_, NullID);
        vertToHalfedge_.resize(V_, NullID);
        edgeToHalfedge_.resize(E_, NullID);

        InternalSetTwin(a1, b2);
        InternalSetTwin(b0, NullID);
        InternalSetTwin(b1, a1t);

        InternalSetVert(a1, v3, {        });
        InternalSetVert(b0, v3, { NullID });
        InternalSetVert(b1, v1, { a1     });
        InternalSetVert(b2, v2, {        });

        InternalSetEdge(a1, e3, { NullID });
        InternalSetEdge(b0, e4, { NullID });
        InternalSetEdge(b1, e1, { a1     });
        InternalSetEdge(b2, e3, {        });
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

        halfedgeToHead_.resize(H_, NullID);
        halfedgeToTwin_.resize(H_, NullID);
        halfedgeToEdge_.resize(H_, NullID);
        vertToHalfedge_.resize(V_, NullID);
        edgeToHalfedge_.resize(E_, NullID);

        InternalSetTwin(a0, d1);
        InternalSetTwin(a1, c0);
        InternalSetTwin(b0, c1);
        InternalSetTwin(b1, d0);
        InternalSetTwin(c2, a1t);
        InternalSetTwin(d2, b1t);

        InternalSetVert(a1, v4, { NullID });
        InternalSetVert(b1, v4, {        });
        InternalSetVert(c0, v3, {        });
        InternalSetVert(c1, v4, {        });
        InternalSetVert(c2, v2, { a1     });
        InternalSetVert(d0, v1, {        });
        InternalSetVert(d1, v4, {        });
        InternalSetVert(d2, v0, { b1     });

        InternalSetEdge(a1, e6, { NullID });
        InternalSetEdge(b0, e5, { NullID });
        InternalSetEdge(b1, e7, { NullID });
        InternalSetEdge(c0, e6, {        });
        InternalSetEdge(c1, e5, {        });
        InternalSetEdge(c2, e2, { a1     });
        InternalSetEdge(d0, e7, {        });
        InternalSetEdge(d1, e4, { b0     });
        InternalSetEdge(d2, e0, { b1     });
    }
}

void HalfedgeMesh::SplitFace(int f)
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

    halfedgeToHead_.resize(H_, NullID);
    halfedgeToTwin_.resize(H_, NullID);
    halfedgeToEdge_.resize(H_, NullID);
    vertToHalfedge_.resize(V_, NullID);
    edgeToHalfedge_.resize(E_, NullID);

    InternalSetTwin(a1, b2);
    InternalSetTwin(a2, c1);
    InternalSetTwin(b0, a1t);
    InternalSetTwin(b1, c2);
    InternalSetTwin(c0, a2t);

    InternalSetVert(a2, v3, { NullID });
    InternalSetVert(b0, v1, { a1     });
    InternalSetVert(b1, v2, {        });
    InternalSetVert(b2, v3, {        });
    InternalSetVert(c0, v2, { a2     });
    InternalSetVert(c1, v0, {        });
    InternalSetVert(c2, v3, {        });

    InternalSetEdge(a1, e4, { NullID });
    InternalSetEdge(a2, e3, { NullID });
    InternalSetEdge(b0, e1, { a1     });
    InternalSetEdge(b1, e5, { NullID });
    InternalSetEdge(b2, e4, {        });
    InternalSetEdge(c0, e2, { a2     });
    InternalSetEdge(c1, e3, {        });
    InternalSetEdge(c2, e5, {        });
}

void HalfedgeMesh::CollapseEdge(int h)
{
    // Collect all necessary info before any modification

    const int a0 = h;
    const int a1 = Succ(a0);
    const int a2 = Succ(a1);

    const int b0 = Twin(a0);
    const int b1 = Succ(b0);
    const int b2 = Succ(b1);

    const int e0 = Edge(a0);
    const int e1 = Edge(a1);
    const int e2 = Edge(a2);
    const int e3 = Edge(b1);
    const int e4 = Edge(b2);

    const int a1t = Twin(a1);
    const int a2t = Twin(a2);
    const int b1t = Twin(b1);
    const int b2t = Twin(b2);

    const int v0 = Vert(a0);
    const int v1 = Vert(a1);
    const int v2 = Vert(a2);
    const int v3 = Vert(b2);

    const int fa = Face(a0);
    const int fb = Face(b0);

    thread_local std::vector<int> halfEdgesOutgoingV0;
    halfEdgesOutgoingV0.clear();
    ForEachOutgoingHalfedge(v0, [&](int hv0)
    {
        if(hv0 != a0 && hv0 != b1)
        {
            halfEdgesOutgoingV0.push_back(hv0);
        }
    });

    thread_local std::vector<int> halfEdgesOutgoingV1;
    halfEdgesOutgoingV1.clear();
    ForEachOutgoingHalfedge(v1, [&](int hv1)
    {
        if(hv1 != b0 && hv1 != a1)
        {
            halfEdgesOutgoingV1.push_back(hv1);
        }
    });
    
    thread_local std::vector<int> halfEdgesOutgoingV2;
    halfEdgesOutgoingV2.clear();
    ForEachOutgoingHalfedge(v2, [&](int hv2)
    {
        if(hv2 != a2)
        {
            halfEdgesOutgoingV2.push_back(hv2);
        }
    });

    thread_local std::vector<int> halfEdgesOutgoingV3;
    halfEdgesOutgoingV3.clear();
    if(v3 != NullID)
    {
        ForEachOutgoingHalfedge(v3, [&](int hv3)
        {
            if(hv3 != b2)
            {
                halfEdgesOutgoingV3.push_back(hv3);
            }
        });
    }

    // Update internal references. Use PendingDeleteID for elements to be removed.

    // H -> V

    for(int v : halfEdgesOutgoingV1)
    {
        halfedgeToHead_[v] = v0;
    }

    // H -> T

    InternalSetTwin(a1t, a2t);
    InternalSetTwin(b1t, b2t);

    // H -> E

    if(b2t != NullID)
    {
        halfedgeToEdge_[b2t] = e3;
    }
    if(a1t != NullID)
    {
        halfedgeToEdge_[a1t] = e2;
    }

    // E -> H

    StaticVector<int, 5> toBeRemovedEdges;

    edgeToHalfedge_[e2] = (std::max)(a1t, a2t);
    if(edgeToHalfedge_[e2] == NullID)
    {
        toBeRemovedEdges.push_back(e2);
    }
    if(e3 != NullID)
    {
        edgeToHalfedge_[e3] = (std::max)(b1t, b2t);
        if(edgeToHalfedge_[e3] == NullID)
        {
            toBeRemovedEdges.push_back(e3);
        }
    }
    toBeRemovedEdges.push_back(e0);
    toBeRemovedEdges.push_back(e1);
    if(e4 != NullID)
    {
        toBeRemovedEdges.push_back(e4);
    }

    // V -> H

    StaticVector<int, 4> toBeRemovedVertices;

    {
        int preferredHalfedgeV0 = NullID;
        for(int hv : halfEdgesOutgoingV0)
        {
            if(Twin(hv) == NullID)
            {
                preferredHalfedgeV0 = hv;
                break;
            }
        }
        if(preferredHalfedgeV0 == NullID)
        {
            for(int hv : halfEdgesOutgoingV1)
            {
                if(Twin(hv) == NullID)
                {
                    preferredHalfedgeV0 = hv;
                    break;
                }
            }
        }

        if(preferredHalfedgeV0 != NullID)
        {
            vertToHalfedge_[v0] = preferredHalfedgeV0;
        }
        else if(!halfEdgesOutgoingV0.empty())
        {
            vertToHalfedge_[v0] = halfEdgesOutgoingV0[0];
        }
        else if(!halfEdgesOutgoingV1.empty())
        {
            vertToHalfedge_[v0] = halfEdgesOutgoingV1[0];
        }
        else
        {
            toBeRemovedVertices.push_back(v0);
        }
    }

    toBeRemovedVertices.push_back(v1);

    {
        int preferredHalfedgeV2 = NullID;
        for(int hv : halfEdgesOutgoingV2)
        {
            if(Twin(hv) == NullID)
            {
                preferredHalfedgeV2 = hv;
                break;
            }
        }
        if(preferredHalfedgeV2 != NullID)
        {
            vertToHalfedge_[v2] = preferredHalfedgeV2;
        }
        else if(!halfEdgesOutgoingV2.empty())
        {
            vertToHalfedge_[v2] = halfEdgesOutgoingV2[0];
        }
        else
        {
            toBeRemovedVertices.push_back(v2);
        }
    }

    if(v3 != NullID)
    {
        int preferredHalfedgeV3 = NullID;
        for(int hv : halfEdgesOutgoingV3)
        {
            if(Twin(hv) == NullID)
            {
                preferredHalfedgeV3 = hv;
                break;
            }
        }
        if(preferredHalfedgeV3 != NullID)
        {
            vertToHalfedge_[v3] = preferredHalfedgeV3;
        }
        else if(!halfEdgesOutgoingV3.empty())
        {
            vertToHalfedge_[v3] = halfEdgesOutgoingV3[0];
        }
        else
        {
            toBeRemovedVertices.push_back(v3);
        }
    }

    // All unreferenced elements are now marked with PendingDeleteID. Time to remove them!

    for(int v : std::views::reverse(toBeRemovedVertices))
    {
        MarkVertexForDeletion(v);
    }

    for(int e : std::views::reverse(toBeRemovedEdges))
    {
        MarkEdgeForDeletion(e);
    }

    MarkFaceForDeletion(fa);
    if(fb != NullID)
    {
        MarkFaceForDeletion(fb);
    }
}

bool HalfedgeMesh::IsCompacted() const
{
    return verticesMarkedForDeletion_.empty() && edgesMarkedForDeletion_.empty() && facesMarkedForDeletion_.empty();
}

void HalfedgeMesh::Compact()
{
    this->Compact([](MoveType, int, int) {});
}

bool HalfedgeMesh::CheckSanity() const
{
    for(int h = 0; h < H(); ++h)
    {
        if(Twin(h) == PendingDeleteID)
        {
            continue;
        }

        const int twin = Twin(h);
        if(twin != NullID && Twin(twin) != h)
        {
            return false;
        }

        if(Twin(h) == h)
        {
            return false;
        }
    }

    for(int v = 0; v < V(); ++v)
    {
        if(VertToHalfedge(v) == PendingDeleteID)
        {
            continue;
        }
        if(VertToHalfedge(v) == NullID || Vert(VertToHalfedge(v)) != v)
        {
            return false;
        }
    }

    for(int e = 0; e < E(); ++e)
    {
        if(EdgeToHalfedge(e) == PendingDeleteID)
        {
            continue;
        }
        if(EdgeToHalfedge(e) == NullID || Edge(EdgeToHalfedge(e)) != e)
        {
            return false;
        }
    }

    return true;
}

void HalfedgeMesh::InternalSetEdge(int newH, int e, Span<int> oldHs)
{
    // By setting oldHs = { NullID }, the caller must ensure that edgeToHalfedge[e] has not been initialized
    // and will always be set to newH in this call.
    assert(!(oldHs.size() > 1 && oldHs[0] == NullID));
    assert(!(!oldHs.IsEmpty() && oldHs[0] == NullID && edgeToHalfedge_[e] != NullID));
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

void HalfedgeMesh::InternalSetVert(int newH, int v, Span<int> oldHs)
{
    assert(!(oldHs.size() > 1 && oldHs[0] == NullID));
    assert(!(!oldHs.IsEmpty() && oldHs[0] == NullID && vertToHalfedge_[v] != NullID));
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

void HalfedgeMesh::InternalSetTwin(int h0, int h1)
{
    if(h0 != NullID) { halfedgeToTwin_[h0] = h1; }
    if(h1 != NullID) { halfedgeToTwin_[h1] = h0; }
}

void HalfedgeMesh::MarkVertexForDeletion(int v)
{
    assert(vertToHalfedge_[v] != PendingDeleteID);
    vertToHalfedge_[v] = PendingDeleteID;
    verticesMarkedForDeletion_.push_back(v);
}

void HalfedgeMesh::MarkEdgeForDeletion(int e)
{
    assert(edgeToHalfedge_[e] != PendingDeleteID);
    edgeToHalfedge_[e] = PendingDeleteID;
    edgesMarkedForDeletion_.push_back(e);
}

void HalfedgeMesh::MarkFaceForDeletion(int f)
{
    for(int i = 0; i < 3; ++i)
    {
        assert(halfedgeToHead_[3 * f + i] != PendingDeleteID);
        assert(halfedgeToTwin_[3 * f + i] != PendingDeleteID);
        assert(halfedgeToEdge_[3 * f + i] != PendingDeleteID);
        halfedgeToHead_[3 * f + i] = PendingDeleteID;
        halfedgeToTwin_[3 * f + i] = PendingDeleteID;
        halfedgeToEdge_[3 * f + i] = PendingDeleteID;
    }
    facesMarkedForDeletion_.push_back(f);
}

RTRC_GEO_END
