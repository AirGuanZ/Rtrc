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

    template<typename Scalar, int Axis>
    Eigen::SparseMatrix<Scalar> BuildFaceGradientMatrix_Axis(const FlatHalfedgeMesh &mesh, Span<Vector3<Scalar>> positions)
    {
        std::vector<Eigen::Triplet<Scalar>> triplets;
        for(int f = 0; f < mesh.F(); ++f)
        {
            const int v0 = mesh.Vert(3 * f + 0);
            const int v1 = mesh.Vert(3 * f + 1);
            const int v2 = mesh.Vert(3 * f + 2);
            const Vector3<Scalar> &p0 = positions[v0];
            const Vector3<Scalar> &p1 = positions[v1];
            const Vector3<Scalar> &p2 = positions[v2];
            const Vector3<Scalar> e0 = p1 - p0;
            const Vector3<Scalar> e1 = p2 - p1;
            const Vector3<Scalar> e2 = p0 - p2;

            const Vector3<Scalar> normal = Normalize(Cross(e2, e0));
            const Scalar area = ComputeTriangleAreaFromEdgeLengths(Length(e0), Length(e1), Length(e2));
            const Scalar areaFactor = 1 / (2 * area);

            const Scalar c0 = areaFactor * Cross(normal, e1)[Axis];
            const Scalar c1 = areaFactor * Cross(normal, e2)[Axis];
            const Scalar c2 = areaFactor * Cross(normal, e0)[Axis];

            triplets.push_back(Eigen::Triplet<Scalar>(f, v0, c0));
            triplets.push_back(Eigen::Triplet<Scalar>(f, v1, c1));
            triplets.push_back(Eigen::Triplet<Scalar>(f, v2, c2));
        }

        Eigen::SparseMatrix<Scalar> ret(mesh.F(), mesh.V());
        ret.setFromTriplets(triplets.begin(), triplets.end());
        return ret;
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

template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildFaceGradientMatrix_X(const FlatHalfedgeMesh &mesh, Span<Vector3<Scalar>> positions)
{
    return DiscreteOperatorDetail::BuildFaceGradientMatrix_Axis<Scalar, 0>(mesh, positions);
}

template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildFaceGradientMatrix_Y(const FlatHalfedgeMesh &mesh, Span<Vector3<Scalar>> positions)
{
    return DiscreteOperatorDetail::BuildFaceGradientMatrix_Axis<Scalar, 1>(mesh, positions);
}

template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildFaceGradientMatrix_Z(const FlatHalfedgeMesh &mesh, Span<Vector3<Scalar>> positions)
{
    return DiscreteOperatorDetail::BuildFaceGradientMatrix_Axis<Scalar, 2>(mesh, positions);
}

template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildVertexDivergenceMatrix(const FlatHalfedgeMesh &mesh, Span<Vector3<Scalar>> positions)
{
    std::vector<Eigen::Triplet<Scalar>> triplets;
    for(int v = 0; v < mesh.V(); ++v)
    {
        mesh.ForEachOutgoingHalfedge(v, [&](int h0)
        {
            const int f = h0 / 3;

            const int h1 = mesh.Succ(h0);
            const int h2 = mesh.Succ(h1);
            const int v0 = mesh.Vert(h0);
            const int v1 = mesh.Vert(h1);
            const int v2 = mesh.Vert(h2);

            const Vector3<Scalar> &p0 = positions[v0];
            const Vector3<Scalar> &p1 = positions[v1];
            const Vector3<Scalar> &p2 = positions[v2];

            const Scalar cotTheta0 = DiscreteOperatorDetail::Cotan(p2, p0, p1);
            const Scalar cotTheta1 = DiscreteOperatorDetail::Cotan(p1, p0, p2);

            const Vector3<Scalar> coef = Scalar(0.5) * (cotTheta0 * (p1 - p0) + cotTheta1 * (p2 - p0));
            triplets.push_back({ v, 3 * f + 0, coef.x });
            triplets.push_back({ v, 3 * f + 1, coef.y });
            triplets.push_back({ v, 3 * f + 2, coef.z });
        });
    }

    Eigen::SparseMatrix<Scalar> ret(mesh.V(), mesh.F() * 3);
    ret.setFromTriplets(triplets.begin(), triplets.end());
    return ret;
}

template <typename Scalar>
Eigen::SparseMatrix<Scalar> BuildCotanLaplacianMatrix(
    const FlatHalfedgeMesh    &mesh,
    Span<Vector3<Scalar>>      positions,
    CotanLaplacianBoundaryType boundaryConditions)
{
    const bool dirichlet = boundaryConditions == CotanLaplacianBoundaryType::Dirichlet;

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

template Eigen::SparseMatrix<float> BuildFaceGradientMatrix_X(const FlatHalfedgeMesh &, Span<Vector3<float>>);
template Eigen::SparseMatrix<double> BuildFaceGradientMatrix_X(const FlatHalfedgeMesh &, Span<Vector3<double>>);
template Eigen::SparseMatrix<float> BuildFaceGradientMatrix_Y(const FlatHalfedgeMesh &, Span<Vector3<float>>);
template Eigen::SparseMatrix<double> BuildFaceGradientMatrix_Y(const FlatHalfedgeMesh &, Span<Vector3<double>>);
template Eigen::SparseMatrix<float> BuildFaceGradientMatrix_Z(const FlatHalfedgeMesh &, Span<Vector3<float>>);
template Eigen::SparseMatrix<double> BuildFaceGradientMatrix_Z(const FlatHalfedgeMesh &, Span<Vector3<double>>);

template Eigen::SparseMatrix<float> BuildVertexDivergenceMatrix(const FlatHalfedgeMesh &mesh, Span<Vector3<float>>);
template Eigen::SparseMatrix<double> BuildVertexDivergenceMatrix(const FlatHalfedgeMesh &mesh, Span<Vector3<double>>);

template Eigen::SparseMatrix<float>  BuildCotanLaplacianMatrix<float>(const FlatHalfedgeMesh &, Span<Vector3<float>>, CotanLaplacianBoundaryType);
template Eigen::SparseMatrix<double> BuildCotanLaplacianMatrix<double>(const FlatHalfedgeMesh &, Span<Vector3<double>>, CotanLaplacianBoundaryType);

RTRC_END
