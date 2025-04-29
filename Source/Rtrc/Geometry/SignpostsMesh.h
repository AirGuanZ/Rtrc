#pragma once

#include <Rtrc/Core/Math/Vector.h>
#include <Rtrc/Geometry/HalfedgeMesh.h>

RTRC_GEO_BEGIN

template<typename T>
class SignpostsMesh
{
public:

    static SignpostsMesh Build(Span<uint32_t> indices, Span<Vector3<T>> positions, T edgeLengthTolerance = 1e-7);
    static SignpostsMesh Build(HalfedgeMesh connectivity, Span<Vector3<T>> positions, T edgeLengthTolerance = 1e-7);

    SignpostsMesh() = default;

    const HalfedgeMesh &GetConnectivity() const { return connectivity_; }

    T GetEdgeLength(int e) const { return lengths_[e]; }
    T GetSumAngle  (int v) const { return thetas_[v];  }
    T GetDirection (int v, int e) const;
    
    /* Keep flipping until for each non-boundary edge e, the sum of the two opposite angles of
     * e's neighboring faces is no more than 180 degrees.
     * 
     *                /-- |--------- beta
     *    alpha   /---    |   ---/
     *         ---------- |--/
     *
     *     alpha + beta <= pi * (1 + tolerance)
     *
     * Theoretically, this process will always terminate. The tolerance is included to handle numerical errors.
     */
    void FlipToDelaunayTriangulation(T tolerance = T(0.03));

private:

    static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);

    HalfedgeMesh connectivity_;

    std::vector<T>               lengths_; // edge length
    std::vector<T>               thetas_;  // vertex -> sum of original angles
    std::vector<std::pair<T, T>> phis_;    // edge -> (directional angle, twin directional angle)
};

template <typename T>
T SignpostsMesh<T>::GetDirection(int v, int e) const
{
    const int h = connectivity_.EdgeToHalfedge(e);
    if(connectivity_.Head(h) == v)
    {
        return phis_[e].first;
    }
    assert(connectivity_.Tail(h) == v);
    return phis_[e].second;
}

RTRC_GEO_END
