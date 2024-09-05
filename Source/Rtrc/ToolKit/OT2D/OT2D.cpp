#include <Rtrc/ToolKit/OT2D/OT2D.h>

RTRC_BEGIN

void OT2D::Initialize(int resolution, Span<unsigned char> cachedInternalState)
{
#if RTRC_HAS_OT2D_IMPL
    impl_.Initialize(resolution, cachedInternalState);
#else
    throw Exception("Not implemented");
#endif
}

std::vector<unsigned char> OT2D::GenerateCachedInternalState() const
{
#if RTRC_HAS_OT2D_IMPL
    return impl_.GenerateInternalStateCache();
#else
    throw Exception("Not implemented");
#endif
}

Image<Vector2d> OT2D::Solve(const Image<double>& sourceDensity)
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
    const auto displacement = impl_.Solve(n, input);

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
