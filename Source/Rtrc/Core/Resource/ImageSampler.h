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
    static void Sample(const Image<P> &image, const Vector2<F> &uv)
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
        return {};
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
            // TODO
            return Vector2i();
        }
    }
};

RTRC_END
