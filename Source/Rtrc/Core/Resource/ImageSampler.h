#pragma once

#include <Rtrc/Core/Resource/Image.h>

RTRC_BEGIN

template<typename P, typename F>
P LinearInterpolate(const P &u0v0, const P &u1v0, const P &u0v1, const P &u1v1, F u, F v)
{
    return (u0v0 * (1 - u) + u1v0 * u) * (1 - v) + (u0v1 * (1 - u) + u1v1 * u) * v;
}

class ImageSampler
{
public:

    enum WrapMode
    {
        Clamp,
        Repeat,
    };

    enum FilterMode
    {
        Point,
        Bilinear,
    };

    template<WrapMode WM, FilterMode FM, typename P, typename F>
    static P Sample(const Image<P> &image, const Vector2<F> &uv)
    {
        if constexpr(FM == Point)
        {
            return SamplePoint<WM>(image, uv);
        }
        else
        {
            return SampleBilinear<WM>(image, uv);
        }
    }

private:

    static int EuclideamRem(int a, int b)
    {
        int r = a % b;
        if(r < 0)
        {
            r += std::abs(b);
        }
        return r;
    }

    template<WrapMode WM, typename P, typename F>
    static P SamplePoint(const Image<P> &image, const Vector2<F> &uv)
    {
        const int x = static_cast<int>(uv.x * image.GetSWidth());
        const int y = static_cast<int>(uv.y * image.GetSHeight());
        const Vector2i coord = Wrap<WM>(image, { x, y });
        return image(coord.x, coord.y);
    }

    template<WrapMode WM, typename P, typename F>
    static P SampleBilinear(const Image<P> &image, const Vector2<F> &uv)
    {
        const F fu = uv.x * image.GetSWidth();
        const F fv = uv.y * image.GetSHeight();

        const int pu = static_cast<int>(fu);
        const int pv = static_cast<int>(fv);

        const int dpu = (fu > pu + F(0.5)) ? 1 : -1;
        const int dpv = (fv > pv + F(0.5)) ? 1 : -1;

        const int apu = pu + dpu;
        const int apv = pv + dpv;

        const F du = (std::min)(std::abs(fu - pu - F(0.5)), F(1));
        const F dv = (std::min)(std::abs(fv - pv - F(0.5)), F(1));

        auto tex = [&](int x, int y)
        {
            const Vector2i coord = Wrap<WM>(image, { x, y });
            return image(coord.x, coord.y);
        };
        const auto pupv   = tex(pu, pv);
        const auto apupv  = tex(apu, pv);
        const auto puapv  = tex(pu, apv);
        const auto apuapv = tex(apu, apv);

        return Rtrc::LinearInterpolate(
            pupv, apupv, puapv, apuapv, du, dv);
    }

    template<WrapMode WM, typename P>
    static Vector2i Wrap(const Image<P>& image, const Vector2i &coord)
    {
        if constexpr(WM == Clamp)
        {
            return Vector2i(
                std::clamp(coord.x, 0, image.GetSWidthMinusOne()),
                std::clamp(coord.y, 0, image.GetSHeightMinusOne()));
        }
        else
        {
            static_assert(WM == Repeat);
            return Vector2i(
                ImageSampler::EuclideamRem(coord.x, image.GetSWidth()),
                ImageSampler::EuclideamRem(coord.y, image.GetSHeight()));
        }
    }
};

RTRC_END
