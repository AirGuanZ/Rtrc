#include <Rtrc/Core/Math/Exact/Expansion.h>
#include <Rtrc/Core/Math/Exact/Predicates.h>
#include <Rtrc/Core/Math/Exact/Vector.h>

#if _MSC_VER
#pragma fp_contract(off)
#pragma fenv_access(off)
#pragma float_control(precise, on)
#else
#warning "float_control are not implemented. Expansion arithemetic operations may become unreliable."
#endif

RTRC_BEGIN

namespace PredicatesDetail
{

    bool IsOne(const Expansion &e)
    {
        return e.GetLength() == 1 && e.GetItems()[0] == 1.0;
    }

    template<typename T>
    int GetSign(const T &v)
    {
        return v < 0 ? -1 : (v > 0 ? 1 : 0);
    }

    template<typename T>
    T Abs(const T &v)
    {
        return v < 0 ? -v : v;
    }

    template<typename T>
    T Square(const T &v)
    {
        return v * v;
    }

    template <typename T>
    int Orient2DExact(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc)
    {
        const auto A = SExpansion(pa[0]) - SExpansion(pc[0]);
        const auto B = SExpansion(pb[0]) - SExpansion(pc[0]);
        const auto C = SExpansion(pa[1]) - SExpansion(pc[1]);
        const auto D = SExpansion(pb[1]) - SExpansion(pc[1]);
        return (A * D - B * C).GetSign();
    }

    template <typename T>
    int Orient3DExact(const Vector3<T> &pa, const Vector3<T> &pb, const Vector3<T> &pc, const Vector3<T> &pd)
    {
        const auto adx = SExpansion(pa[0]) - SExpansion(pd[0]);
        const auto bdx = SExpansion(pb[0]) - SExpansion(pd[0]);
        const auto cdx = SExpansion(pc[0]) - SExpansion(pd[0]);
        const auto ady = SExpansion(pa[1]) - SExpansion(pd[1]);
        const auto bdy = SExpansion(pb[1]) - SExpansion(pd[1]);
        const auto cdy = SExpansion(pc[1]) - SExpansion(pd[1]);
        const auto adz = SExpansion(pa[2]) - SExpansion(pd[2]);
        const auto bdz = SExpansion(pb[2]) - SExpansion(pd[2]);
        const auto cdz = SExpansion(pc[2]) - SExpansion(pd[2]);

        const auto bdxcdy = bdx * cdy;
        const auto cdxbdy = cdx * bdy;

        const auto cdxady = cdx * ady;
        const auto adxcdy = adx * cdy;

        const auto adxbdy = adx * bdy;
        const auto bdxady = bdx * ady;

        const auto det = adz * (bdxcdy - cdxbdy)
                       + bdz * (cdxady - adxcdy)
                       + cdz * (adxbdy - bdxady);

        return det.GetSign();
    }

    template<typename T>
    int InCircle2DExact(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc, const Vector2<T> &pd)
    {
        const auto adx = SExpansion(pa[0]) - SExpansion(pd[0]);
        const auto ady = SExpansion(pa[1]) - SExpansion(pd[1]);
        const auto bdx = SExpansion(pb[0]) - SExpansion(pd[0]);
        const auto bdy = SExpansion(pb[1]) - SExpansion(pd[1]);
        const auto cdx = SExpansion(pc[0]) - SExpansion(pd[0]);
        const auto cdy = SExpansion(pc[1]) - SExpansion(pd[1]);

        const auto abdet = adx * bdy - bdx * ady;
        const auto bcdet = bdx * cdy - cdx * bdy;
        const auto cadet = cdx * ady - adx * cdy;
        const auto alift = adx * adx + ady * ady;
        const auto blift = bdx * bdx + bdy * bdy;
        const auto clift = cdx * cdx + cdy * cdy;

        return (alift * bcdet + blift * cadet + clift * abdet).GetSign();
    }

    // Orient2D for fast rejection. When returning 0, the result is not reliable.
    int UncertainOrient2D(const Vector2d &pa, const Vector2d &pb, const Vector2d &pc)
    {
        const double detL = (pa[0] - pc[0]) * (pb[1] - pc[1]);
        const double detR = (pa[1] - pc[1]) * (pb[0] - pc[0]);
        const double det = detL - detR;

        double detSum;
        if(detL > 0)
        {
            if(detR <= 0)
            {
                return GetSign(det);
            }
            detSum = detL + detR;
        }
        else if(detL < 0)
        {
            if(detR >= 0)
            {
                return GetSign(det);
            }
            detSum = -detL - detR;
        }
        else
        {
            return GetSign(det);
        }

        const double unitError = double(0.5) * std::numeric_limits<double>::epsilon();
        const double errorBoundFactor = (3.0 + 16.0 * unitError) * unitError;
        const double errorBound = errorBoundFactor * detSum;
        if(det > errorBound || -det > errorBound)
        {
            return GetSign(det);
        }

        return 0;
    }

} // namespace PredicatesDetail

int Orient2D(const Expansion2 &pa, const Expansion2 &pb, const Expansion2 &pc)
{
    return Orient2D(&pa.x, &pb.x, &pc.x);
}

int Orient2D(const Expansion *pa, const Expansion *pb, const Expansion *pc)
{
    const auto A = pa[0] - pc[0];
    const auto B = pb[0] - pc[0];
    const auto C = pa[1] - pc[1];
    const auto D = pb[1] - pc[1];
    return (A * D - B * C).GetSign();
}

int Orient2DHomogeneous(const Expansion3 &pa, const Expansion3 &pb, const Expansion3 &pc)
{
    // Fast path for regular coordinate

    if(PredicatesDetail::IsOne(pa.z) && PredicatesDetail::IsOne(pb.z) && PredicatesDetail::IsOne(pc.z))
    {
        const auto A = pa[0] - pc[0];
        const auto B = pb[0] - pc[0];
        const auto C = pa[1] - pc[1];
        const auto D = pb[1] - pc[1];
        return (A * D - B * C).GetSign();
    }

    // Homogeneous case

    const int signFactor = pa[2].GetSign() * pb[2].GetSign();
    const auto A = pa[0] * pc[2] - pc[0] * pa[2];
    const auto B = pb[0] * pc[2] - pc[0] * pb[2];
    const auto C = pa[1] * pc[2] - pc[1] * pa[2];
    const auto D = pb[1] * pc[2] - pc[1] * pb[2];
    return signFactor * (A * D - B * C).GetSign();
}

int Orient3D(const Expansion3 &pa, const Expansion3 &pb, const Expansion3 &pc, const Expansion3 &pd)
{
    const auto adx = pa[0] - pd[0];
    const auto bdx = pb[0] - pd[0];
    const auto cdx = pc[0] - pd[0];
    const auto ady = pa[1] - pd[1];
    const auto bdy = pb[1] - pd[1];
    const auto cdy = pc[1] - pd[1];
    const auto adz = pa[2] - pd[2];
    const auto bdz = pb[2] - pd[2];
    const auto cdz = pc[2] - pd[2];

    const auto bdxcdy = bdx * cdy;
    const auto cdxbdy = cdx * bdy;

    const auto cdxady = cdx * ady;
    const auto adxcdy = adx * cdy;

    const auto adxbdy = adx * bdy;
    const auto bdxady = bdx * ady;

    const auto det = adz * (bdxcdy - cdxbdy)
                   + bdz * (cdxady - adxcdy)
                   + cdz * (adxbdy - bdxady);

    return det.GetSign();
}

int InCircle2D(const Expansion2 &pa, const Expansion2 &pb, const Expansion2 &pc, const Expansion2 &pd)
{
    return InCircle2D(&pa.x, &pb.x, &pc.x, &pd.x);
}

int InCircle2D(const Expansion *pa, const Expansion *pb, const Expansion *pc, const Expansion *pd)
{
    const auto adx = pa[0] - pd[0];
    const auto ady = pa[1] - pd[1];
    const auto bdx = pb[0] - pd[0];
    const auto bdy = pb[1] - pd[1];
    const auto cdx = pc[0] - pd[0];
    const auto cdy = pc[1] - pd[1];

    const auto abdet = adx * bdy - bdx * ady;
    const auto bcdet = bdx * cdy - cdx * bdy;
    const auto cadet = cdx * ady - adx * cdy;
    const auto alift = adx * adx + ady * ady;
    const auto blift = bdx * bdx + bdy * bdy;
    const auto clift = cdx * cdx + cdy * cdy;

    return (alift * bcdet + blift * cadet + clift * abdet).GetSign();
}

int InCircle2DHomogeneous(const Expansion3 &pa, const Expansion3 &pb, const Expansion3 &pc, const Expansion3 &pd)
{
    if(PredicatesDetail::IsOne(pa.z) &&
       PredicatesDetail::IsOne(pb.z) &&
       PredicatesDetail::IsOne(pc.z) &&
       PredicatesDetail::IsOne(pd.z))
    {
        return InCircle2D(&pa.x, &pb.x, &pc.x, &pd.x);
    }

    using PredicatesDetail::Square;

    // Multiple pa.z^2 * pd.z^2 to the first row, pb.z^2 * pd.z^2 to the second row, etc...
    // | a b c |
    // | d e f |
    // | g h i |

    const auto az2 = pa[2] * pa[2];
    const auto bz2 = pb[2] * pb[2];
    const auto cz2 = pc[2] * pc[2];
    const auto dz2 = pd[2] * pd[2];

    const auto a = pa.x * pa.z * dz2 - pd.x * pd.z * az2;
    const auto b = pa.y * pa.z * dz2 - pd.y * pd.z * az2;

    const auto d = pb.x * pb.z * dz2 - pd.x * pd.z * bz2;
    const auto e = pb.y * pb.z * dz2 - pd.y * pd.z * bz2;

    const auto g = pc.x * pc.z * dz2 - pd.x * pd.z * cz2;
    const auto h = pc.y * pc.z * dz2 - pd.y * pd.z * cz2;

    const auto c = PredicatesDetail::Square(pa.x * pd.z - pd.x * pa.z) + PredicatesDetail::Square(pa.y * pd.z - pd.y * pa.z);
    const auto f = PredicatesDetail::Square(pb.x * pd.z - pd.x * pb.z) + PredicatesDetail::Square(pb.y * pd.z - pd.y * pb.z);
    const auto i = PredicatesDetail::Square(pc.x * pd.z - pd.x * pc.z) + PredicatesDetail::Square(pc.y * pd.z - pd.y * pc.z);

    const auto sc = d * h - e * g;
    const auto sf = a * h - b * g;
    const auto si = a * e - b * d;

    return (c * sc - f * sf + i * si).GetSign();
}

template <typename T>
int Orient2D(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc)
{
    if(const int approx = Rtrc::Orient2DApprox(pa, pb, pc))
    {
        return approx;
    }
    return PredicatesDetail::Orient2DExact(pa, pb, pc);
}

template int Orient2D(const Vector2<float> &, const Vector2<float> &, const Vector2<float> &);
template int Orient2D(const Vector2<double> &, const Vector2<double> &, const Vector2<double> &);
template int Orient2D(const Vector2<ExpansionUtility::float_100_27> &, const Vector2<ExpansionUtility::float_100_27> &, const Vector2<ExpansionUtility::float_100_27> &);
template int Orient2D(const Vector2<ExpansionUtility::float_200_55> &, const Vector2<ExpansionUtility::float_200_55> &, const Vector2<ExpansionUtility::float_200_55> &);

template<typename T>
int Orient3D(const Vector3<T> &pa, const Vector3<T> &pb, const Vector3<T> &pc, const Vector3<T> &pd)
{
    if(const int approx = Rtrc::Orient3DApprox(pa, pb, pc, pd))
    {
        return approx;
    }
    return PredicatesDetail::Orient3DExact(pa, pb, pc, pd);
}

template int Orient3D(const Vector3<float> &, const Vector3<float> &, const Vector3<float> &, const Vector3<float> &);
template int Orient3D(const Vector3<double> &, const Vector3<double> &, const Vector3<double> &, const Vector3<double> &);
template int Orient3D(const Vector3<ExpansionUtility::float_100_27> &, const Vector3<ExpansionUtility::float_100_27> &, const Vector3<ExpansionUtility::float_100_27> &, const Vector3<ExpansionUtility::float_100_27> &);
template int Orient3D(const Vector3<ExpansionUtility::float_200_55> &, const Vector3<ExpansionUtility::float_200_55> &, const Vector3<ExpansionUtility::float_200_55> &, const Vector3<ExpansionUtility::float_200_55> &);

template<typename T>
int InCircle2D(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc, const Vector2<T> &pd)
{
    const T adx = pa[0] - pd[0];
    const T bdx = pb[0] - pd[0];
    const T cdx = pc[0] - pd[0];
    const T ady = pa[1] - pd[1];
    const T bdy = pb[1] - pd[1];
    const T cdy = pc[1] - pd[1];

    const T bdxcdy = bdx * cdy;
    const T cdxbdy = cdx * bdy;
    const T alift  = adx * adx + ady * ady;

    const T cdxady = cdx * ady;
    const T adxcdy = adx * cdy;
    const T blift  = bdx * bdx + bdy * bdy;

    const T adxbdy = adx * bdy;
    const T bdxady = bdx * ady;
    const T clift  = cdx * cdx + cdy * cdy;

    const T det = alift * (bdxcdy - cdxbdy)
                + blift * (cdxady - adxcdy)
                + clift * (adxbdy - bdxady);

    const T permanent = (PredicatesDetail::Abs(bdxcdy) + PredicatesDetail::Abs(cdxbdy)) * alift
                      + (PredicatesDetail::Abs(cdxady) + PredicatesDetail::Abs(adxcdy)) * blift
                      + (PredicatesDetail::Abs(adxbdy) + PredicatesDetail::Abs(bdxady)) * clift;
    const T unitError = T(0.5) * std::numeric_limits<T>::epsilon();
    const T errorBoundFactor = (10.0 + 96.0 * unitError) * unitError;
    const T errorBound = errorBoundFactor * permanent;
    if(det > errorBound || -det > errorBound)
    {
        return PredicatesDetail::GetSign(det);
    }

    return PredicatesDetail::InCircle2DExact(pa, pb, pc, pd);
}

template int InCircle2D(const Vector2<float> &, const Vector2<float> &, const Vector2<float> &, const Vector2<float> &);
template int InCircle2D(const Vector2<double> &, const Vector2<double> &, const Vector2<double> &, const Vector2<double> &);
template int InCircle2D(const Vector2<ExpansionUtility::float_100_27> &, const Vector2<ExpansionUtility::float_100_27> &, const Vector2<ExpansionUtility::float_100_27> &, const Vector2<ExpansionUtility::float_100_27> &);
template int InCircle2D(const Vector2<ExpansionUtility::float_200_55> &, const Vector2<ExpansionUtility::float_200_55> &, const Vector2<ExpansionUtility::float_200_55> &, const Vector2<ExpansionUtility::float_200_55> &);

template<typename T>
int Orient2DApprox(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc)
{
    const T detL = (pa[0] - pc[0]) * (pb[1] - pc[1]);
    const T detR = (pa[1] - pc[1]) * (pb[0] - pc[0]);
    const T det = detL - detR;

    T detSum;
    if(detL > 0)
    {
        if(detR <= 0)
        {
            return PredicatesDetail::GetSign(det);
        }
        detSum = detL + detR;
    }
    else if(detL < 0)
    {
        if(detR >= 0)
        {
            return PredicatesDetail::GetSign(det);
        }
        detSum = -detL - detR;
    }
    else
    {
        return PredicatesDetail::GetSign(det);
    }

    const T unitError = T(0.5) * std::numeric_limits<T>::epsilon();
    const T errorBoundFactor = (3.0 + 16.0 * unitError) * unitError;
    const T errorBound = errorBoundFactor * detSum;
    if(det > errorBound || -det > errorBound)
    {
        return PredicatesDetail::GetSign(det);
    }

    return 0;
}

template int Orient2DApprox(const Vector2<float>&, const Vector2<float>&, const Vector2<float>&);
template int Orient2DApprox(const Vector2<double>&, const Vector2<double>&, const Vector2<double>&);
template int Orient2DApprox(const Vector2<ExpansionUtility::float_100_27>&, const Vector2<ExpansionUtility::float_100_27>&, const Vector2<ExpansionUtility::float_100_27>&);
template int Orient2DApprox(const Vector2<ExpansionUtility::float_200_55>&, const Vector2<ExpansionUtility::float_200_55>&, const Vector2<ExpansionUtility::float_200_55>&);

template<typename T>
int Orient3DApprox(const Vector3<T> &pa, const Vector3<T> &pb, const Vector3<T> &pc, const Vector3<T> &pd)
{
    const T adx = pa[0] - pd[0];
    const T bdx = pb[0] - pd[0];
    const T cdx = pc[0] - pd[0];
    const T ady = pa[1] - pd[1];
    const T bdy = pb[1] - pd[1];
    const T cdy = pc[1] - pd[1];
    const T adz = pa[2] - pd[2];
    const T bdz = pb[2] - pd[2];
    const T cdz = pc[2] - pd[2];

    const T bdxcdy = bdx * cdy;
    const T cdxbdy = cdx * bdy;

    const T cdxady = cdx * ady;
    const T adxcdy = adx * cdy;

    const T adxbdy = adx * bdy;
    const T bdxady = bdx * ady;
    
    const T det = adz * (bdxcdy - cdxbdy)
                + bdz * (cdxady - adxcdy)
                + cdz * (adxbdy - bdxady);

    const T permanent = (PredicatesDetail::Abs(bdxcdy) + PredicatesDetail::Abs(cdxbdy)) * PredicatesDetail::Abs(adz)
                      + (PredicatesDetail::Abs(cdxady) + PredicatesDetail::Abs(adxcdy)) * PredicatesDetail::Abs(bdz)
                      + (PredicatesDetail::Abs(adxbdy) + PredicatesDetail::Abs(bdxady)) * PredicatesDetail::Abs(cdz);
    const T unitError = T(0.5) * std::numeric_limits<T>::epsilon();
    const T errorBoundFactor = (7.0 + 56.0 * unitError) * unitError;
    const T errorBound = errorBoundFactor * permanent;
    if(det > errorBound || -det > errorBound)
    {
        return PredicatesDetail::GetSign(det);
    }

    return 0;
}

template int Orient3DApprox(const Vector3<float>&, const Vector3<float>&, const Vector3<float>&, const Vector3<float>&);
template int Orient3DApprox(const Vector3<double>&, const Vector3<double>&, const Vector3<double>&, const Vector3<double>&);
template int Orient3DApprox(const Vector3<ExpansionUtility::float_100_27>&, const Vector3<ExpansionUtility::float_100_27>&, const Vector3<ExpansionUtility::float_100_27>&, const Vector3<ExpansionUtility::float_100_27>&);
template int Orient3DApprox(const Vector3<ExpansionUtility::float_200_55>&, const Vector3<ExpansionUtility::float_200_55>&, const Vector3<ExpansionUtility::float_200_55>&, const Vector3<ExpansionUtility::float_200_55>&);

template <typename T>
bool AreCoLinear(const Vector3<T> &a, const Vector3<T> &b, const Vector3<T> &c)
{
    if(a == b || b == c || c == a)
    {
        return true;
    }

    if(PredicatesDetail::UncertainOrient2D(Vector2d(a.x, a.y), Vector2d(b.x, b.y), Vector2d(c.x, c.y)) != 0 ||
       PredicatesDetail::UncertainOrient2D(Vector2d(a.x, a.z), Vector2d(b.x, b.z), Vector2d(c.x, c.z)) != 0 ||
       PredicatesDetail::UncertainOrient2D(Vector2d(a.y, a.z), Vector2d(b.y, b.z), Vector2d(c.y, c.z)) != 0)
    {
        return false;
    }

    const auto ux = SExpansion(b.x) - SExpansion(a.x);
    const auto uy = SExpansion(b.y) - SExpansion(a.y);
    const auto uz = SExpansion(b.z) - SExpansion(a.z);

    const auto vx = SExpansion(c.x) - SExpansion(a.x);
    const auto vy = SExpansion(c.y) - SExpansion(a.y);
    const auto vz = SExpansion(c.z) - SExpansion(a.z);

    const auto dot = ux * vx + uy * vy + uz * vz;
    const auto lu = ux * ux + uy * uy + uz * uz;
    const auto lv = vx * vx + vy * vy + vz * vz;

    return dot * dot == lu * lv;
}

template bool AreCoLinear(const Vector3<float> &, const Vector3<float> &, const Vector3<float> &);
template bool AreCoLinear(const Vector3<double> &, const Vector3<double> &, const Vector3<double> &);

RTRC_END
