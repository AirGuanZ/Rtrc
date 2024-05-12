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

private:

    int H_ = 0;
    int V_ = 0;
    int E_ = 0;
    int F_ = 0;

    std::vector<int> halfEdgeToTwin_;
    std::vector<int> halfEdgeToEdge_;
    std::vector<int> halfEdgeToHead_;

    std::vector<int> vertToHalfEdge_;
    std::vector<int> edgeToHalfEdge_;
    std::vector<int> faceToHalfEdge_;
};

RTRC_END
