#include <Rtrc/Core/Math/Common.h>
#include <Rtrc/Geometry/DiscreteOperators.h>

RTRC_BEGIN

namespace DiscreteOperatorDetail
{

    // cot<ab, ac>
    template<typename T>
    T Cotan(const Vector3<T> &a, const Vector3<T> &b, const Vector3<T> &c)
    {
        const Vector3<T> ab = b - a;
        const Vector3<T> ac = c - a;
        return Dot(ab, ac) / Length(Cross(ab, ac));
    }

} // namespace DiscreteOperatorDetail

template <typename Scalar>
Eigen::VectorX<Scalar> BuildVertexAreaVector(const FlatHalfedgeMesh &mesh, Span<Vector3<Scalar>> positions)
{
    Eigen::VectorX<Scalar> ret(mesh.V());
    for(int v = 0; v < mesh.V(); ++v)
    {
        Scalar area = 0;
        mesh.ForEachHalfedge<true>(v, [&](int h)
        {
            if(mesh.Vert(h) != v)
            {
                return;
            }
            const int h0 = h;
            const int h1 = mesh.Succ(h0);
            const int h2 = mesh.Succ(h1);
            const int v0 = mesh.Vert(h0);
            const int v1 = mesh.Vert(h1);
            const int v2 = mesh.Vert(h2);
            const Vector3<Scalar> p0 = positions[v0];
            const Vector3<Scalar> p1 = positions[v1];
            const Vector3<Scalar> p2 = positions[v2];
            const Scalar l0 = Length(p0 - p1);
            const Scalar l1 = Length(p1 - p2);
            const Scalar l2 = Length(p2 - p0);
            area += ComputeTriangleAreaFromEdgeLengths(l0, l1, l2);
        });
        ret[v] = area;
    }
    return ret;
}

template <typename Scalar>
Eigen::SparseMatrix<Scalar> BuildVertexAreaDiagonalMatrix(const FlatHalfedgeMesh &mesh, Span<Vector3<Scalar>> positions)
{
    const auto vector = BuildVertexAreaVector(mesh, positions);

    std::vector<Eigen::Triplet<Scalar>> triplets;
    triplets.reserve(vector.rows());
    for(int v = 0; v < mesh.V(); ++v)
    {
        triplets.push_back(Eigen::Triplet<Scalar>(v, v, vector(v)));
    }

    Eigen::SparseMatrix<Scalar> ret(mesh.V(), mesh.V());
    ret.setFromTriplets(triplets.begin(), triplets.end());
    return ret;
}

template <typename Scalar>
Eigen::SparseMatrix<Scalar> BuildCotanLaplacianMatrix(
    const FlatHalfedgeMesh     &mesh,
    Span<Vector3<Scalar>>       positions,
    LaplacianBoundaryConditions boundaryConditions)
{
    const bool dirichlet = boundaryConditions == LaplacianBoundaryConditions::zeroDirichlet;

    auto ComputeCotan = [&](int v, int n, int nPrev, int nSucc)
    {
        const Vector3<Scalar> &pv = positions[v];
        const Vector3<Scalar> &pn = positions[n];

        Scalar cotPrev = 0;
        if(nPrev >= 0)
        {
            const Vector3<Scalar> pPrev = positions[nPrev];
            cotPrev = DiscreteOperatorDetail::Cotan<Scalar>(pPrev, pv, pn);
        }

        Scalar cotSucc = 0;
        if(nSucc >= 0)
        {
            const Vector3<Scalar> pSucc = positions[nSucc];
            cotSucc = DiscreteOperatorDetail::Cotan<Scalar>(pSucc, pv, pn);
        }

        return Scalar(0.5) * (cotPrev + cotSucc);
    };

    std::vector<Eigen::Triplet<Scalar>> triplets;
    for(int v = 0; v < mesh.V(); ++v)
    {
        if(dirichlet && mesh.IsVertOnBoundary(v))
        {
            continue;
        }

        int h0 = mesh.VertToHalfedge(v);
        assert(mesh.Vert(h0) == v);

        Scalar sumW = 0;
        int h = h0;
        while(true)
        {
            assert(mesh.Vert(h) == v);
            const int n = mesh.Vert(mesh.Succ(h));

            const int nPrev = mesh.Vert(mesh.Prev(mesh.Twin(h)));
            const int nSucc = mesh.Vert(mesh.Prev(h));

            const Scalar w = ComputeCotan(v, n, nPrev, nSucc);
            sumW += w;

            if(!dirichlet || !mesh.IsVertOnBoundary(n))
            {
                triplets.push_back(Eigen::Triplet<Scalar>(v, n, w));
            }

            if(const int nh = mesh.Twin(mesh.Prev(h)); nh < 0) // We are on the another boundary
            {
                const Scalar wLast = ComputeCotan(v, nSucc, n, -1);
                sumW += wLast;

                if(!dirichlet)
                {
                    triplets.push_back(Eigen::Triplet<Scalar>(v, nSucc, wLast));
                }

                break;
            }
            else
            {
                h = nh;
            }

            if(h == h0)
            {
                break;
            }
        }

        triplets.push_back(Eigen::Triplet<Scalar>(v, v, -sumW));
    }

    Eigen::SparseMatrix<Scalar> ret(mesh.V(), mesh.V());
    ret.setFromTriplets(triplets.begin(), triplets.end());
    return ret;
}


template Eigen::VectorX<float> BuildVertexAreaVector<float>(const FlatHalfedgeMesh &, Span<Vector3<float>>);
template Eigen::VectorX<double> BuildVertexAreaVector<double>(const FlatHalfedgeMesh &, Span<Vector3<double>>);

template Eigen::SparseMatrix<float> BuildVertexAreaDiagonalMatrix(const FlatHalfedgeMesh &, Span<Vector3<float>>);
template Eigen::SparseMatrix<double> BuildVertexAreaDiagonalMatrix(const FlatHalfedgeMesh &, Span<Vector3<double>>);

template Eigen::SparseMatrix<float>  BuildCotanLaplacianMatrix<float>(const FlatHalfedgeMesh &, Span<Vector3<float>>, LaplacianBoundaryConditions);
template Eigen::SparseMatrix<double> BuildCotanLaplacianMatrix<double>(const FlatHalfedgeMesh &, Span<Vector3<double>>, LaplacianBoundaryConditions);

RTRC_END
