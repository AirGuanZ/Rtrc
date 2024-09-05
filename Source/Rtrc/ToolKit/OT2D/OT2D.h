#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Resource/Image.h>

#if __has_include(<Rtrc/ToolKit/OT2D/OT2DImpl.h>)
#define RTRC_HAS_OT2D_IMPL 1
#include <Rtrc/ToolKit/OT2D/OT2DImpl.h>
#else
#define RTRC_HAS_OT2D_IMPL 0
#endif

RTRC_BEGIN

class OT2D
{
public:

    // Optional. Can be used to optimize laplacian pre factorization.
    void Initialize(int resolution, Span<unsigned char> cachedInternalState = {});

    std::vector<unsigned char> GenerateCachedInternalState() const;

    // source density must be square
    Image<Vector2d> Solve(const Image<double> &sourceDensity);

private:

#if RTRC_HAS_OT2D_IMPL
    OT2DImpl impl_;
#endif
};

RTRC_END
