#include <numbers>

#include <Rtrc/Core/Math/Common.h>
#include <Rtrc/Geometry/SignpostsMesh.h>

RTRC_BEGIN

template <typename T>
SignpostsMesh<T> SignpostsMesh<T>::Build(Span<uint32_t> indices, Span<Vector3<T>> positions, T edgeLengthTolerance)
{
    SignpostsMesh ret;

    // Build connectivity

    ret.connectivity = FlatHalfedgeMesh::Build(indices);

    // Compute edge length

    ret.lengths.resize(ret.connectivity.E());
    for(int e = 0; e < ret.connectivity.E(); ++e)
    {
        const int h = ret.connectivity.EdgeToHalfedge(e);
        const int v0 = ret.connectivity.Vert(h);
        const int v1 = ret.connectivity.Vert(ret.connectivity.Succ(h));
        const T length = Rtrc::Length(positions[v0] - positions[v1]);
        ret.lengths[e] = length;
    }

    // Bias lengths globally so that for every triangle abc is non-degenerate (ab + bc > ac + edgeLengthTolerance)

    T eps = 0;
    for(int f = 0; f < ret.connectivity.F(); ++f)
    {
        const int e0 = ret.connectivity.Edge(3 * f + 0);
        const int e1 = ret.connectivity.Edge(3 * f + 1);
        const int e2 = ret.connectivity.Edge(3 * f + 2);
        const T l0 = ret.lengths[e0];
        const T l1 = ret.lengths[e1];
        const T l2 = ret.lengths[e2];
        const T localEps = (std::max)(
        {
            edgeLengthTolerance - l0 - l1 + l2,
            edgeLengthTolerance - l0 - l2 + l1,
            edgeLengthTolerance - l1 - l2 + l0,
        });
        eps = (std::max)(eps, localEps);
    }

    for(T &l : ret.lengths)
    {
        l += eps;
    }

    // Sum angles at each vertex

    std::vector<T> halfEdgeToAngles(ret.connectivity.H(), -100);
    ret.thetas.resize(ret.connectivity.V());
    for(int v = 0; v < ret.connectivity.V(); ++v)
    {
        T sum = 0;
        ret.connectivity.ForEachHalfedge<true>([&](int h)
        {
            if(ret.connectivity.Vert(h) != v)
            {
                return;
            }

            const int h0 = h;
            const int h1 = ret.connectivity.Succ(h0);
            const int h2 = ret.connectivity.Succ(h1);

            const int e0 = ret.connectivity.Edge(h0);
            const int e1 = ret.connectivity.Edge(h1);
            const int e2 = ret.connectivity.Edge(h2);

            const T l0 = ret.lengths[e0];
            const T l1 = ret.lengths[e1];
            const T l2 = ret.lengths[e2];

            const T cosAngle = (l0 * l0 + l2 * l2 - l1 * l1) / (2 * l0 * l2);
            const T angle = std::acos(cosAngle);
            halfEdgeToAngles[h] = angle;
            sum += angle;
        });
        ret.thetas[v] = sum;
    }

    // Check sanity

#if RTRC_DEBUG
    for(T angle : halfEdgeToAngles)
    {
        // Assumes there is no degenerate triangle
        assert(angle > 0);
    }
#endif

    // Evaluate relative angles

    ret.phis.resize(ret.connectivity.E(), { -100, -100 });
    for(int v = 0; v < ret.connectivity.V(); ++v)
    {
        const T theta = ret.thetas[v];
        T curr = 0;
        ret.connectivity.ForEachHalfedge<true>([&](int h)
        {
            const int e = ret.connectivity.Edge(h);
            if(ret.connectivity.EdgeToHalfedge(e) == h)
            {
                ret.phis[e].first = curr;
            }
            else
            {
                assert(ret.connectivity.EdgeToHalfedge(e) == ret.connectivity.Twin(h));
                ret.phis[e].second = curr;
            }
            curr = Rtrc::Saturate(curr + halfEdgeToAngles[h] / theta);
        });
    }

    // Check sanity

#if RTRC_DEBUG
    for(auto &phiPair : ret.phis)
    {
        assert(phiPair.first > 0 && phiPair.second > 0);
    }
#endif

    return ret;
}

template <typename T>
void SignpostsMesh<T>::FlipToDelaunayTriangulation(T tolerance)
{
    assert(tolerance >= 0);
    const T angleThreshold = std::numbers::pi_v<T> * (T(1) + tolerance);

    auto &m = connectivity;
    std::set<int> activeEdges = std::views::iota(0, m.E()) | std::ranges::to<std::set<int>>();

    while(!activeEdges.empty())
    {
        const int e0 = *activeEdges.begin();
        activeEdges.erase(e0);

        const int ha0 = m.EdgeToHalfedge(e0);
        const int hb0 = m.Twin(ha0);
        if(hb0 < 0)
        {
            continue;
        }

        const int e1 = m.Edge(m.Succ(ha0));
        const int e2 = m.Edge(m.Prev(ha0));
        const int e3 = m.Edge(m.Succ(hb0));
        const int e4 = m.Edge(m.Prev(hb0));

        const int v2 = m.Vert(m.Prev(ha0));
        const int v3 = m.Vert(m.Prev(hb0));

        T l0 = lengths[e0];
        const T l1 = lengths[e1];
        const T l2 = lengths[e2];
        const T l3 = lengths[e3];
        const T l4 = lengths[e4];

        // Compute angle <ab, ac>
        auto ComputeAngle = [&](T ab, T ac, T bc)
        {
            const T cosAngle = (ab * ab + ac * ac - bc * bc) / (T(2) * ab * ac);
            return std::acos(Rtrc::Clamp(cosAngle, T(-1), T(1)));
        };

        const T alpha = ComputeAngle(l1, l2, l0);
        const T beta  = ComputeAngle(l3, l4, l0);
        if(alpha + beta <= angleThreshold)
        {
            continue;
        }

        m.FlipEdge(e0);

        l0 = std::sqrt((l1 * l3 + l2 * l4) * (l1 * l2 + l3 * l4) / (l1 * l4 + l2 * l3));
        lengths[e0] = l0;

        T angle1 = ComputeAngle(l0, l2, l3) / thetas[v2];
        T angle2 = ComputeAngle(l0, l4, l1) / thetas[v3];
        assert(m.Vert(m.EdgeToHalfedge(e0)) == v2 || m.Vert(m.EdgeToHalfedge(e0)) == v3);
        if(m.Vert(m.EdgeToHalfedge(e0)) == v3)
        {
            std::swap(angle1, angle2);
        }
        phis[e0].first = angle1;
        phis[e0].second = angle2;

        activeEdges.insert(e1);
        activeEdges.insert(e2);
        activeEdges.insert(e3);
        activeEdges.insert(e4);
    }
}

template<> class SignpostsMesh<float>;
template<> class SignpostsMesh<double>;

RTRC_END
