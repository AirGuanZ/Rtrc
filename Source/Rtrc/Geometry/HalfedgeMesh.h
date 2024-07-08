#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/EnumFlags.h>

RTRC_GEO_BEGIN

namespace FlatHalfedgeMeshDetail
{

    enum class BuildOptionFlagBit
    {
        ThrowOnInvalidInput = 1 << 0,
    };
    RTRC_DEFINE_ENUM_FLAGS(BuildOptionFlagBit)

} // namespace FlatHalfedgeMeshDetail

class HalfedgeMesh
{
public:

    static constexpr int NullID = -1;
    static constexpr int PendingDeleteID = -2;

    using BuildOptions = FlatHalfedgeMeshDetail::EnumFlagsBuildOptionFlagBit;
    using enum FlatHalfedgeMeshDetail::BuildOptionFlagBit;

    // Construct a halfedge mesh structure from the provided triangle mesh.
    // Includes a basic manifold check focusing solely on connectivity.
    // This method also assumes that each vertex is contained by at least one face.
    static HalfedgeMesh Build(Span<uint32_t> indices, BuildOptions options = ThrowOnInvalidInput);

    bool IsEmpty() const { return H_ == 0; }

    int H() const { return H_; }
    int V() const { return V_; }
    int E() const { return E_; }
    int F() const { return F_; }

    bool IsVertValid    (int v) const { return 0 <= v && v < V_ && vertToHalfedge_[v]     != PendingDeleteID; }
    bool IsFaceValid    (int f) const { return 0 <= f && f < F_ && halfedgeToEdge_[3 * f] != PendingDeleteID; }
    bool IsEdgeValid    (int e) const { return 0 <= e && e < E_ && edgeToHalfedge_[e]     != PendingDeleteID; }
    bool IsHalfedgeValid(int h) const { return 0 <= h && h < H_ && halfedgeToHead_[h]     != PendingDeleteID; }

    int Succ(int h) const { return h != NullID ? ((h / 3 * 3) + (h + 1) % 3) : NullID; }
    int Prev(int h) const { return h != NullID ? ((h / 3 * 3) + (h + 2) % 3) : NullID; }
    int Twin(int h) const { return h != NullID ? halfedgeToTwin_[h] : NullID; }
    int Edge(int h) const { return h != NullID ? halfedgeToEdge_[h] : NullID; }
    int Vert(int h) const { return h != NullID ? halfedgeToHead_[h] : NullID; }
    int Face(int h) const { return h != NullID ? (h / 3) : NullID; }
    int Rawi(int h) const { return h; }

    int Head(int h) const { return Vert(h); }
    int Tail(int h) const { return Vert(Succ(h)); }

    int VertToHalfedge(int v) const { return v != NullID ? vertToHalfedge_[v] : NullID; }
    int EdgeToHalfedge(int e) const { return e != NullID ? edgeToHalfedge_[e] : NullID; }
    int FaceToHalfedge(int f) const { return f != NullID ? (3 * f) : NullID; }

    bool IsEdgeOnBoundary(int e) const { return Twin(EdgeToHalfedge(e)) == NullID; }
    bool IsVertOnBoundary(int v) const { return Twin(VertToHalfedge(v)) == NullID; }

    // If Unique is true, only one of each pair of twin halfedges will be used, with the outgoing one being preferred.
    // func is called with the index of each halfedge.
    // The returned type of func must be either void or bool.
    // When returning bool, false means immediately exiting the traversal.
    template <bool Unique, typename Func>
    void ForEachHalfedge(int v, const Func &func) const;

    // The returned type of func must be either void or bool.
    // When returning bool, false means immediately exiting the traversal.
    template<typename Func>
    void ForEachOutgoingHalfedge(int v, const Func &func) const;

    // Returns the first outgoing halfedge given by ForEachOutgoingHalfedge
    int GetFirstOutgoingHalfedge(int v) const;

    // func is called with the index of each neighboring vertex.
    // The returned type of func must be either void or bool.
    // When returning bool, false means immediately exiting the traversal.
    template<typename Func>
    void ForEachNeighbor(int v, const Func &func) const;

    // Perform an atomic edge flip operation.
    // 'e' points to the newly flipped edge afterward.
    void FlipEdge(int e);

    // Insert a new vertex v on half-edge h. The new vertex v will always be the original value of V().
    // It is guaranteed that Vert(h) does not change, and Vert(Succ(h)) equals v after the insertion.
    void SplitEdge(int h);

    // Insert a new vertex v into face f, subdividing f into 3 new faces
    // The new vertex will always be the original value of V().
    void SplitFace(int f);

    // Merge vertex Tail(h) into Head(h). h and its corresponding edge will be marked for deletion.
    // Note that this method will leave hole(s) in element arrays. Call `Compact` to remove these holes.
    void CollapseEdge(int h);

    bool IsCompacted() const;

    enum MoveType
    {
        MoveVertex,
        MoveEdge,
        MoveFace
    };

    // Remove elements marked for deletion so that the halfedge mesh becomes compacted again.
    // During this process, valid elements may be moved to fill the holes.
    // The user can provide a custom function to monitor these movements.
    //
    // `func(MoveType type, int movedElement, int targetHole)` is called when the specific movement is finished.
    template<typename Func>
    void Compact(const Func &func);

    void Compact();

    bool CheckSanity() const;

private:

    // Set Edge(newH) to e.
    // If edgeToHalfedge[e] matches any one of the oldHs, edgeToHalfedge[e] will be set to newH.
    void InternalSetEdge(int newH, int e, Span<int> oldHs);
    void InternalSetVert(int newH, int v, Span<int> oldHs);
    void InternalSetTwin(int h0, int h1);

    void MarkVertexForDeletion(int v);
    void MarkEdgeForDeletion(int e);
    void MarkFaceForDeletion(int f);

    int H_ = 0;
    int V_ = 0;
    int E_ = 0;
    int F_ = 0;

    std::vector<int> facesMarkedForDeletion_;
    std::vector<int> verticesMarkedForDeletion_;
    std::vector<int> edgesMarkedForDeletion_;

    std::vector<int> halfedgeToTwin_;
    std::vector<int> halfedgeToEdge_;
    std::vector<int> halfedgeToHead_;

    std::vector<int> vertToHalfedge_; // One of the halfedges outgoing from v. The boundary one is preferred, if present.
    std::vector<int> edgeToHalfedge_;
};

template <bool Unique, typename Func>
void HalfedgeMesh::ForEachHalfedge(int v, const Func &func) const
{
    int h0 = VertToHalfedge(v);
    assert(Vert(h0) == v);

    auto callFunc = [&](int h)
    {
        if constexpr(std::is_convertible_v<decltype(func(h)), bool>)
        {
            return static_cast<bool>(func(h));
        }
        else
        {
            func(h);
            return true;
        }
    };

    int h = h0;
    while(true)
    {
        if(!callFunc(h))
        {
            return;
        }
        h = Prev(h);
        if constexpr(!Unique)
        {
            if(!callFunc(h))
            {
                return;
            }
        }
        const int nh = Twin(h);
        if(nh < 0)
        {
            if constexpr(Unique)
            {
                callFunc(h);
            }
            return;
        }
        h = nh;
        if(h == h0)
        {
            return;
        }
    }
}

template <typename Func>
void HalfedgeMesh::ForEachOutgoingHalfedge(int v, const Func &func) const
{
    this->ForEachHalfedge<true>(v, [&](int h)
    {
        if constexpr(std::is_convertible_v<decltype(func(h)), bool>)
        {
            if(this->Vert(h) == v)
            {
                return static_cast<bool>(func(h));
            }
            return true;
        }
        else
        {
            if(this->Vert(h) == v)
            {
                func(h);
            }
        }
    });
}

inline int HalfedgeMesh::GetFirstOutgoingHalfedge(int v) const
{
    return VertToHalfedge(v);
}

template <typename Func>
void HalfedgeMesh::ForEachNeighbor(int v, const Func &func) const
{
    this->ForEachHalfedge<true>(v, [&](int h)
    {
        const int nh = Vert(h) == v ? Succ(h) : h;
        const int vn = Vert(nh);
        if constexpr(std::is_convertible_v<decltype(func(vn)), bool>)
        {
            return static_cast<bool>(func(vn));
        }
        else
        {
            func(vn);
        }
    });
}

template<typename Func>
void HalfedgeMesh::Compact(const Func &func)
{
    std::ranges::sort(verticesMarkedForDeletion_, std::greater{});
    for(int v : verticesMarkedForDeletion_)
    {
        RTRC_SCOPE_EXIT
        {
            vertToHalfedge_.pop_back();
            --V_;
        };

        if(v == V_ - 1)
        {
            continue;
        }

        ForEachOutgoingHalfedge(V_ - 1, [&](int h)
        {
            // Be careful! Ensure that the modifications in the lambda do not break the halfedge traversal.
            halfedgeToHead_[h] = v;
        });
        vertToHalfedge_[v] = vertToHalfedge_[V_ - 1];

        func(MoveVertex, V_ - 1, v);
    }
    verticesMarkedForDeletion_.clear();

    std::ranges::sort(edgesMarkedForDeletion_, std::greater{});
    for(int e : edgesMarkedForDeletion_)
    {
        RTRC_SCOPE_EXIT
        {
            edgeToHalfedge_.pop_back();
            --E_;
        };

        if(e == E_ - 1)
        {
            continue;
        }

        const int h0 = edgeToHalfedge_[E_ - 1];
        const int h1 = Twin(h0);
        assert(IsHalfedgeValid(h0));
        halfedgeToEdge_[h0] = e;
        if(h1 != NullID)
        {
            halfedgeToEdge_[h1] = e;
        }
        edgeToHalfedge_[e] = h0;

        func(MoveEdge, E_ - 1, e);
    }
    edgesMarkedForDeletion_.clear();

    std::ranges::sort(facesMarkedForDeletion_, std::greater{});
    for(int f : facesMarkedForDeletion_)
    {
        RTRC_SCOPE_EXIT
        {
            const size_t newSize = halfedgeToHead_.size() - 3;
            halfedgeToHead_.resize(newSize);
            halfedgeToEdge_.resize(newSize);
            halfedgeToTwin_.resize(newSize);
            --F_;
            H_ -= 3;
        };

        if(f == F_ - 1)
        {
            continue;
        }

        const int a0 = 3 * f + 0;
        const int a1 = 3 * f + 1;
        const int a2 = 3 * f + 2;
        const int b0 = 3 * (F_ - 1) + 0;
        const int b1 = 3 * (F_ - 1) + 1;
        const int b2 = 3 * (F_ - 1) + 2;

        const int b0t = Twin(b0);
        const int b1t = Twin(b1);
        const int b2t = Twin(b2);

        const int b0v = Vert(b0);
        const int b1v = Vert(b1);
        const int b2v = Vert(b2);

        const int b0e = Edge(b0);
        const int b1e = Edge(b1);
        const int b2e = Edge(b2);

        InternalSetTwin(a0, b0t);
        InternalSetTwin(a1, b1t);
        InternalSetTwin(a2, b2t);

        InternalSetEdge(a0, b0e, { b0 });
        InternalSetEdge(a1, b1e, { b1 });
        InternalSetEdge(a2, b2e, { b2 });

        InternalSetVert(a0, b0v, { b0 });
        InternalSetVert(a1, b1v, { b1 });
        InternalSetVert(a2, b2v, { b2 });

        func(MoveFace, F_ - 1, f);
    }
    facesMarkedForDeletion_.clear();
}

RTRC_GEO_END
