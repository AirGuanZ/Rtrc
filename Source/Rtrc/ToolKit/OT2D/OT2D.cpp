#include <Rtrc/ToolKit/OT2D/OT2D.h>

#if __has_include(<Rtrc/ToolKit/OT2D/OT2DImpl.h>)
#define RTRC_HAS_OT2D_IMPL 1
#include <Rtrc/ToolKit/OT2D/OT2DImpl.h>
#else
#define RTRC_HAS_OT2D_IMPL 0
#endif

RTRC_BEGIN

Image<Vector2d> OT2D::Solve(const Image<double>& sourceDensity) const
{
#if RTRC_HAS_OT2D_IMPL
    assert(sourceDensity.GetSWidth() == sourceDensity.GetSHeight());

    const int n = sourceDensity.GetSWidth();
    Eigen::VectorXd input;
    input.resize(n * n);
    for(int x = 0; x < n; ++x)
    {
        for(int y = 0; y < n; ++y)
        {
            input[x * n + y] = sourceDensity(x, y);
        }
    }
    const auto displacement = OT2DImpl{}.Solve(n, input);

    Image<Vector2d> ret(n + 1, n + 1);
    for(int y = 0; y <= n; ++y)
    {
        for(int x = 0; x <= n; ++x)
        {
            ret(x, y) = 1.0 / n * Vector2d(x, y) + displacement(x, y);
        }
    }
    return ret;
#else
    throw Exception("Not implemented");
#endif
}

RTRC_END
