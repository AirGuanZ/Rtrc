#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/EnumFlags.h>

RTRC_BEGIN

namespace FlatHalfEdgeMeshDetail
{

    enum class BuildOptionFlagBit
    {
        ThrowOnNonManifoldInput = 1 << 0,
    };
    RTRC_DEFINE_ENUM_FLAGS(BuildOptionFlagBit)

} // namespace FlatHalfEdgeMeshDetail

class FlatHalfEdgeMesh
{
public:

    using BuildOptions = FlatHalfEdgeMeshDetail::EnumFlagsBuildOptionFlagBit;
    using enum FlatHalfEdgeMeshDetail::BuildOptionFlagBit;

    // Construct a half-edge mesh structure from the provided triangle mesh.
    // Includes a basic manifold check focusing solely on connectivity.
    static FlatHalfEdgeMesh Build(Span<uint32_t> indices, BuildOptions options = ThrowOnNonManifoldInput);

    bool IsEmpty() const { return H_ == 0; }

    int H() const { return H_; }
    int V() const { return V_; }
    int E() const { return E_; }
    int F() const { return F_; }

    int Succ(int h) const { return (h / 3 * 3) + (h + 1) % 3; }
    int Prev(int h) const { return (h / 3 * 3) + (h + 2) % 3; }
    int Twin(int h) const { return halfEdgeToTwin_[h]; }
    int Edge(int h) const { return halfEdgeToEdge_[h]; }
    int Vert(int h) const { return halfEdgeToHead_[h]; }
    int Face(int h) const { return h / 3; }
    int Rawi(int h) const { return h; }

    int VertToHalfEdge(int v) const { return vertToHalfEdge_[v]; }
    int EdgeToHalfEdge(int e) const { return edgeToHalfEdge_[e]; }
    int FaceToHalfEdge(int f) const { return faceToHalfEdge_[f]; }

    // If Unique is true, only one of each pair of twin half-edges will be used, with the outgoing one being preferred.
    // func is called with the index of each half edge.
    template <bool Unique, typename Func>
    void ForEachHalfEdge(int v, const Func &func) const;

    // func is called with the index of each neighboring vertex.
    template<typename Func>
    void ForEachNeighbor(int v, const Func &func) const;

private:

    int H_ = 0;
    int V_ = 0;
    int E_ = 0;
    int F_ = 0;

    std::vector<int> halfEdgeToTwin_;
    std::vector<int> halfEdgeToEdge_;
    std::vector<int> halfEdgeToHead_;

    std::vector<int> vertToHalfEdge_; // One of the half edges outgoing from v. The boundary one is preferred, if present.
    std::vector<int> edgeToHalfEdge_;
    std::vector<int> faceToHalfEdge_;
};

template <bool Unique, typename Func>
void FlatHalfEdgeMesh::ForEachHalfEdge(int v, const Func &func) const
{
    int h0 = VertToHalfEdge(v);
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
void FlatHalfEdgeMesh::ForEachNeighbor(int v, const Func &func) const
{
    this->ForEachHalfEdge<true>(v, [&](int h)
    {
        const int nh = Vert(h) == v ? Succ(h) : h;
        func(Vert(nh));
    });
}

RTRC_END
