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
        using E = Expansion;
        const E detL = (E(pa[0]) - E(pc[0])) * (E(pb[1]) - E(pc[1]));
        const E detR = (E(pa[1]) - E(pc[1])) * (E(pb[0]) - E(pc[1]));
        const E det = detL - detR;
        return det.GetSign();
    }

    template <typename T>
    int Orient3DExact(const Vector3<T> &pa, const Vector3<T> &pb, const Vector3<T> &pc, const Vector3<T> &pd)
    {
        using E = Expansion;

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

} // namespace PredicatesDetail

template <typename T>
int Orient2D(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc)
{
    const T detL = (pa[0] - pc[0]) * (pb[1] - pc[1]);
    const T detR = (pa[1] - pc[1]) * (pb[0] - pc[1]);
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

template<typename T>
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

RTRC_GEO_END
