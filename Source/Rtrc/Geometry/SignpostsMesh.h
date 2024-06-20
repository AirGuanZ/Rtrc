#pragma once

#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Geometry/HalfedgeMesh.h>

RTRC_BEGIN

template<typename T>
class SignpostsMesh
{
public:

    static SignpostsMesh Build(Span<uint32_t> indices, Span<Vector3<T>> positions, T edgeLengthTolerance = 0);

    SignpostsMesh() = default;

    const HalfedgeMesh &GetConnectivity() const { return connectivity; }

    T GetEdgeLength(int e) const { return lengths[e]; }
    T GetSumAngle  (int v) const { return thetas[v];  }
    T GetDirection (int h) const { return phis[h];    }

    /* Keep flipping until for each non-boundary edge e, the sum of the two opposite angles of
     * e's neighboring faces is no more than 180 degrees.
     * 
     *                /-- |--------- beta
     *    alpha   /---    |   ---/
     *         ---------- |--/
     *
     *     alpha + beta <= pi * (1 + tolerance)
     *
     * Theoretically, the algorithm will always terminate. The tolerance is included to handle numerical errors.
     */
    void FlipToDelaunayTriangulation(T tolerance = T(0.03));

private:

    static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);

    HalfedgeMesh connectivity;
    std::vector<T> lengths; // edge length
    std::vector<T> thetas;  // vertex -> sum of original angles
    std::vector<std::pair<T, T>> phis; // edge -> (directional angle, twin directional angle)
};

RTRC_END
