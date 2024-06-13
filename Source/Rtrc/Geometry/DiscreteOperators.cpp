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
Eigen::SparseMatrix<Scalar> BuildCotanLaplacianMatrix(const FlatHalfedgeMesh &mesh, Span<Vector3<Scalar>> positions)
{
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

    Eigen::SparseMatrix<Scalar> ret(mesh.V(), mesh.V());
    for(int v = 0; v < mesh.V(); ++v)
    {
        int h0 = mesh.VertToHalfedge(v);
        assert(mesh.Vert(h0) == v);

        Scalar sumW = 0;
        int h = h0;
        while(true)
        {
            assert(mesh.Vert(h) == v);
            const int n = mesh.Vert(mesh.Succ(h));
            const int nPrev = mesh.Twin(h) >= 0 ? mesh.Vert(mesh.Twin(h)) : -1;
            const int nSucc = mesh.Vert(mesh.Prev(n));
            const Scalar w = ComputeCotan(v, n, nPrev, nSucc);
            sumW += w;
            ret(v, n) = w;

            if(const int nh = mesh.Twin(mesh.Prev(h)); nh < 0)
            {
                const Scalar wLast = ComputeCotan(v, nSucc, n, -1);
                sumW += wLast;
                ret(v, nSucc) = w;
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

        ret(v, v) = -sumW;
    }
    return ret;
}

template<> Eigen::SparseMatrix<float>  BuildCotanLaplacianMatrix<float>(const FlatHalfedgeMesh &, Span<Vector3<float>>);
template<> Eigen::SparseMatrix<double> BuildCotanLaplacianMatrix<double>(const FlatHalfedgeMesh &, Span<Vector3<double>>);

RTRC_END
