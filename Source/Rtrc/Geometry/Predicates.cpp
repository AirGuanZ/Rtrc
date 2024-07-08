#include <Rtrc/Geometry/Expansion.h>
#include <Rtrc/Geometry/Predicates.h>

#if _MSC_VER
#pragma fp_contract(off)
#pragma fenv_access(off)
#pragma float_control(precise, on)
#else
#warning "float_control are not implemented. Expansion arithemetic operations may become unreliable."
#endif

RTRC_GEO_BEGIN

namespace PredicatesDetail
{

    template<typename T>
    int GetSign(T v)
    {
        return v < 0 ? -1 : (v > 0 ? 1 : 0);
    }

    template <typename T>
    int Orient2DExact(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc)
    {
        using E = SExpansion;
        const E A = E(pa[0]) - E(pc[0]);
        const E B = E(pb[0]) - E(pc[0]);
        const E C = E(pa[1]) - E(pc[1]);
        const E D = E(pb[1]) - E(pc[1]);
        return (A * D - B * C).GetSign();
    }

    template <typename T>
    int Orient3DExact(const Vector3<T> &pa, const Vector3<T> &pb, const Vector3<T> &pc, const Vector3<T> &pd)
    {
        using E = SExpansion;

        const E adx = E(pa[0]) - E(pd[0]);
        const E bdx = E(pb[0]) - E(pd[0]);
        const E cdx = E(pc[0]) - E(pd[0]);
        const E ady = E(pa[1]) - E(pd[1]);
        const E bdy = E(pb[1]) - E(pd[1]);
        const E cdy = E(pc[1]) - E(pd[1]);
        const E adz = E(pa[2]) - E(pd[2]);
        const E bdz = E(pb[2]) - E(pd[2]);
        const E cdz = E(pc[2]) - E(pd[2]);

        const E bdxcdy = bdx * cdy;
        const E cdxbdy = cdx * bdy;

        const E cdxady = cdx * ady;
        const E adxcdy = adx * cdy;

        const E adxbdy = adx * bdy;
        const E bdxady = bdx * ady;

        const E det = adz * (bdxcdy - cdxbdy)
                    + bdz * (cdxady - adxcdy)
                    + cdz * (adxbdy - bdxady);

        return det.GetSign();
    }

    template<typename T>
    int InCircleExact(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc, const Vector2<T> &pd)
    {
        using E = SExpansion;

        const E adx = pa[0] - pd[0];
        const E ady = pa[1] - pd[1];
        const E bdx = pb[0] - pd[0];
        const E bdy = pb[1] - pd[1];
        const E cdx = pc[0] - pd[0];
        const E cdy = pc[1] - pd[1];

        const E abdet = adx * bdy - bdx * ady;
        const E bcdet = bdx * cdy - cdx * bdy;
        const E cadet = cdx * ady - adx * cdy;
        const E alift = adx * adx + ady * ady;
        const E blift = bdx * bdx + bdy * bdy;
        const E clift = cdx * cdx + cdy * cdy;

        return (alift * bcdet + blift * cadet + clift * abdet).GetSign();
    }

} // namespace PredicatesDetail

int Orient2D(const Expansion2 &pa, const Expansion2 &pb, const Expansion2 &pc)
{
    return Orient2D(&pa.x, &pb.x, &pc.x);
}

int Orient2D(const Expansion *pa, const Expansion *pb, const Expansion *pc)
{
    using E = SExpansion;
    const E A = pa[0] - pc[0];
    const E B = pb[0] - pc[0];
    const E C = pa[1] - pc[1];
    const E D = pb[1] - pc[1];
    return (A * D - B * C).GetSign();
}

int Orient2DHomogeneous(const Expansion3 &pa, const Expansion3 &pb, const Expansion3 &pc)
{
    using E = SExpansion;

    // Fast path for regular coordinate

    auto IsOne = [](const Expansion &e) { return e.GetLength() == 1 && e.GetItems()[0] == 1.0; };
    if(IsOne(pa.z) && IsOne(pb.z) && IsOne(pc.z))
    {
        const E A = pa[0] - pc[0];
        const E B = pb[0] - pc[0];
        const E C = pa[1] - pc[1];
        const E D = pb[1] - pc[1];
        return (A * D - B * C).GetSign();
    }

    // Homogeneous case

    const E A = pa[0] * pc[2] - pc[0] * pa[2];
    const E B = pb[0] * pc[2] - pc[0] * pb[2];
    const E C = pa[1] * pc[2] - pc[1] * pa[2];
    const E D = pb[1] * pc[2] - pc[1] * pb[2];
    return (A * D - B * C).GetSign();
}

int Orient3D(const Expansion3 &pa, const Expansion3 &pb, const Expansion3 &pc, const Expansion3 &pd)
{
    using E = SExpansion;

    const E adx = pa[0] - pd[0];
    const E bdx = pb[0] - pd[0];
    const E cdx = pc[0] - pd[0];
    const E ady = pa[1] - pd[1];
    const E bdy = pb[1] - pd[1];
    const E cdy = pc[1] - pd[1];
    const E adz = pa[2] - pd[2];
    const E bdz = pb[2] - pd[2];
    const E cdz = pc[2] - pd[2];

    const E bdxcdy = bdx * cdy;
    const E cdxbdy = cdx * bdy;

    const E cdxady = cdx * ady;
    const E adxcdy = adx * cdy;

    const E adxbdy = adx * bdy;
    const E bdxady = bdx * ady;

    const E det = adz * (bdxcdy - cdxbdy)
        + bdz * (cdxady - adxcdy)
        + cdz * (adxbdy - bdxady);

    return det.GetSign();
}

int InCircle(const Expansion2 &pa, const Expansion2 &pb, const Expansion2 &pc, const Expansion2 &pd)
{
    using E = SExpansion;

    const E adx = pa[0] - pd[0];
    const E ady = pa[1] - pd[1];
    const E bdx = pb[0] - pd[0];
    const E bdy = pb[1] - pd[1];
    const E cdx = pc[0] - pd[0];
    const E cdy = pc[1] - pd[1];

    const E abdet = adx * bdy - bdx * ady;
    const E bcdet = bdx * cdy - cdx * bdy;
    const E cadet = cdx * ady - adx * cdy;
    const E alift = adx * adx + ady * ady;
    const E blift = bdx * bdx + bdy * bdy;
    const E clift = cdx * cdx + cdy * cdy;

    return (alift * bcdet + blift * cadet + clift * abdet).GetSign();
}

template <std::floating_point T>
int Orient2D(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc)
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

    constexpr T eps = T(0.5) * std::numeric_limits<T>::epsilon();
    constexpr T errorBoundFactor = (3.0 + 16.0 * eps) * eps;
    const T errorBound = errorBoundFactor * detSum;
    if(det > errorBound || -det > errorBound)
    {
        return PredicatesDetail::GetSign(det);
    }

    return PredicatesDetail::Orient2DExact(pa, pb, pc);
}

template int Orient2D(const Vector2<float> &, const Vector2<float> &, const Vector2<float> &);
template int Orient2D(const Vector2<double> &, const Vector2<double> &, const Vector2<double> &);

template<std::floating_point T>
int Orient3D(const Vector3<T> &pa, const Vector3<T> &pb, const Vector3<T> &pc, const Vector3<T> &pd)
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

    const T permanent = (std::abs(bdxcdy) + std::abs(cdxbdy)) * std::abs(adz)
                      + (std::abs(cdxady) + std::abs(adxcdy)) * std::abs(bdz)
                      + (std::abs(adxbdy) + std::abs(bdxady)) * std::abs(cdz);
    constexpr T eps = T(0.5) * std::numeric_limits<T>::epsilon();
    constexpr T errorBoundFactor = (7.0 + 56.0 * eps) * eps;
    const T errorBound = errorBoundFactor * permanent;
    if(det > errorBound || -det > errorBound)
    {
        return PredicatesDetail::GetSign(det);
    }

    return PredicatesDetail::Orient3DExact(pa, pb, pc, pd);
}

template int Orient3D(const Vector3<float> &, const Vector3<float> &, const Vector3<float> &, const Vector3<float> &);
template int Orient3D(const Vector3<double> &, const Vector3<double> &, const Vector3<double> &, const Vector3<double> &);

template<std::floating_point T>
int InCircle(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc, const Vector2<T> &pd)
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

    const T permanent = (std::abs(bdxcdy) + std::abs(cdxbdy)) * alift
                      + (std::abs(cdxady) + std::abs(adxcdy)) * blift
                      + (std::abs(adxbdy) + std::abs(bdxady)) * clift;
    constexpr T eps = T(0.5) * std::numeric_limits<T>::epsilon();
    constexpr T errorBoundFactor = (10.0 + 96.0 * eps) * eps;
    const T errorBound = errorBoundFactor * permanent;
    if(det > errorBound || -det > errorBound)
    {
        return PredicatesDetail::GetSign(det);
    }

    return PredicatesDetail::InCircleExact(pa, pb, pc, pd);
}

template int InCircle(const Vector2<float> &, const Vector2<float> &, const Vector2<float> &, const Vector2<float> &);
template int InCircle(const Vector2<double> &, const Vector2<double> &, const Vector2<double> &, const Vector2<double> &);

RTRC_GEO_END
