#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/EnumFlags.h>

RTRC_BEGIN

namespace FlatHalfedgeMeshDetail
{

    enum class BuildOptionFlagBit
    {
        ThrowOnInvalidInput = 1 << 0,
    };
    RTRC_DEFINE_ENUM_FLAGS(BuildOptionFlagBit)

} // namespace FlatHalfedgeMeshDetail

class FlatHalfedgeMesh
{
public:

    using BuildOptions = FlatHalfedgeMeshDetail::EnumFlagsBuildOptionFlagBit;
    using enum FlatHalfedgeMeshDetail::BuildOptionFlagBit;

    // Construct a halfedge mesh structure from the provided triangle mesh.
    // Includes a basic manifold check focusing solely on connectivity.
    // This method also assumes that each vertex is contained by at least one face.
    static FlatHalfedgeMesh Build(Span<uint32_t> indices, BuildOptions options = ThrowOnInvalidInput);

    bool IsEmpty() const { return H_ == 0; }

    int H() const { return H_; }
    int V() const { return V_; }
    int E() const { return E_; }
    int F() const { return F_; }

    int Succ(int h) const { return h >= 0 ? ((h / 3 * 3) + (h + 1) % 3) : -1; }
    int Prev(int h) const { return h >= 0 ? ((h / 3 * 3) + (h + 2) % 3) : -1; }
    int Twin(int h) const { return h >= 0 ? halfedgeToTwin_[h] : -1; }
    int Edge(int h) const { return h >= 0 ? halfedgeToEdge_[h] : -1; }
    int Vert(int h) const { return h >= 0 ? halfedgeToHead_[h] : -1; }
    int Face(int h) const { return h >= 0 ? (h / 3) : -1; }
    int Rawi(int h) const { return h; }

    int VertToHalfedge(int v) const { return v >= 0 ? vertToHalfedge_[v] : -1; }
    int EdgeToHalfedge(int e) const { return e >= 0 ? edgeToHalfedge_[e] : -1; }
    int FaceToHalfedge(int f) const { return f >= 0 ? (3 * f) : -1; }

    // If Unique is true, only one of each pair of twin halfedges will be used, with the outgoing one being preferred.
    // func is called with the index of each halfedge.
    template <bool Unique, typename Func>
    void ForEachHalfedge(int v, const Func &func) const;

    // func is called with the index of each neighboring vertex.
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

    bool CheckSanity() const;

private:

    // Set Edge(newH) to e.
    // If edgeToHalfedge[e] matches any one of the oldHs, edgeToHalfedge[e] will be set to newH.
    void InternalSetEdge(int newH, int e, Span<int> oldHs);
    void InternalSetVert(int newH, int v, Span<int> oldHs);
    void InternalSetTwin(int h0, int h1);

    int H_ = 0;
    int V_ = 0;
    int E_ = 0;
    int F_ = 0;

    std::vector<int> halfedgeToTwin_;
    std::vector<int> halfedgeToEdge_;
    std::vector<int> halfedgeToHead_;

    std::vector<int> vertToHalfedge_; // One of the half edges outgoing from v. The boundary one is preferred, if present.
    std::vector<int> edgeToHalfedge_;
};

template <bool Unique, typename Func>
void FlatHalfedgeMesh::ForEachHalfedge(int v, const Func &func) const
{
    int h0 = VertToHalfedge(v);
    assert(Vert(h0) == v);

    int h = h0;
    while(true)
    {
        func(h);
        h = Prev(h);
        if constexpr(!Unique)
        {
            func(h);
        }
        const int nh = Twin(h);
        if(nh < 0)
        {
            if constexpr(Unique)
            {
                func(h);
            }
            break;
        }
        h = nh;
        if(h == h0)
        {
            break;
        }
    }
}

template <typename Func>
void FlatHalfedgeMesh::ForEachNeighbor(int v, const Func &func) const
{
    this->ForEachHalfedge<true>(v, [&](int h)
    {
        const int nh = Vert(h) == v ? Succ(h) : h;
        func(Vert(nh));
    });
}

RTRC_END
