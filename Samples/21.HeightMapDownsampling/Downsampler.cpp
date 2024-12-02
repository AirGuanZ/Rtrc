#include <Rtrc/Core/Math/Rect.h>
#include <Rtrc/Core/Parallel.h>
#include <Rtrc/Core/Resource/ImageSampler.h>

#include "Common.h"

Image<double> Downsample_Native(const Image<double> &source, const Vector2i &targetResolution)
{
    assert(0 < targetResolution.x && targetResolution.x <= source.GetSWidth());
    assert(0 < targetResolution.y && targetResolution.y <= source.GetSHeight());
    Image<double> result(targetResolution.To<uint32_t>());
    for(int y = 0; y < result.GetSHeight(); ++y)
    {
        for(int x = 0; x < result.GetSWidth(); ++x)
        {
            const double u = static_cast<double>(x) / result.GetSWidthMinusOne();
            const double v = static_cast<double>(y) / result.GetSHeightMinusOne();
            result(x, y) = ImageSampler().Sample<ImageSampler::Clamp, ImageSampler::Bilinear>(source, Vector2d(u, v));
        }
    }
    return result;
}

template<bool FullGrid>
static double ComputeGridVolume(
    const Image<double> &heightMap,
    const Vector2i      &rawLowerTexelCoord,
    const Vector2d      &localLower,
    const Vector2d      &localUpper)
{
    const Vector2i lowerTexelCoord = Min(Max(rawLowerTexelCoord, Vector2i(0)), heightMap.GetSSizeMinusOne());
    const Vector2i upperTexelCoord = Min(Max(lowerTexelCoord + Vector2i(1), Vector2i(0)), heightMap.GetSSizeMinusOne());
    const double h00 = heightMap(lowerTexelCoord.x, lowerTexelCoord.y);
    const double h01 = heightMap(upperTexelCoord.x, lowerTexelCoord.y);
    const double h10 = heightMap(lowerTexelCoord.x, upperTexelCoord.y);
    const double h11 = heightMap(upperTexelCoord.x, upperTexelCoord.y);
    const double x0 = localLower.x;
    const double x1 = localUpper.x;
    const double y0 = localLower.y;
    const double y1 = localUpper.y;
    if constexpr(FullGrid)
    {
        return 0.25 * (h00 + h01 + h10 + h11);
    }
    return ((((h00 - h01 - h10 + h11) * (-y0 * y0 + y1 * y1)) / 2 -
             h00 * (y1 - y0) + h01 * (y1 - y0)) * (-x0 * x0 + x1 * x1)) / 2 +
           ((-h00 + h10) * (-y0 * y0 + y1 * y1) * (x1 - x0)) / 2 + h00 * (y1 - y0) * (x1 - x0);
}

template<bool FullGrid>
static double ComputeVolume(const Image<double> &heightMap, const Rectd &rect)
{
    double result = 0;
    const Vector2i iBeg = { static_cast<int>(std::floor(rect.lower.x)), static_cast<int>(std::floor(rect.lower.y)) };
    const Vector2i iEnd = { static_cast<int>(std::ceil(rect.upper.x)),  static_cast<int>(std::ceil(rect.upper.y)) };
    for(int ix = iBeg.x; ix < iEnd.x; ++ix)
    {
        for(int iy = iBeg.y; iy < iEnd.y; ++iy)
        {
            const Vector2d gridLower = Vector2d(ix + 0, iy + 0);
            const Vector2d gridUpper = Vector2d(ix + 1, iy + 1);
            const Vector2d lower = Max(gridLower, rect.lower);
            const Vector2d upper = Min(gridUpper, rect.upper);
            const Rectd inctGrid = { lower, upper };
            if(!inctGrid.IsEmpty())
            {
                result += ComputeGridVolume<FullGrid>(
                    heightMap, { ix, iy }, inctGrid.lower - gridLower, inctGrid.upper - gridLower);
            }
        }
    }
    return result;
}

Image<double> Downsample_PreserveVolume(
    const Image<double> &source,
    const Vector2i      &targetResolution,
    uint32_t             iterations,
    double               centerWeight,
    bool                 initUsingNativeDownsampler)
{
    assert(0 < targetResolution.x && targetResolution.x <= source.GetSWidth());
    assert(0 < targetResolution.y && targetResolution.y <= source.GetSHeight());

    Image<double> referenceNeighborVolumeImage(targetResolution.To<uint32_t>());
    for(int y = 0; y < targetResolution.y; ++y)
    {
        for(int x = 0; x < targetResolution.x; ++x)
        {
            const double u0 = (x - 1) / static_cast<double>(targetResolution.x - 1);
            const double u1 = (x + 1) / static_cast<double>(targetResolution.x - 1);
            const double v0 = (y - 1) / static_cast<double>(targetResolution.y - 1);
            const double v1 = (y + 1) / static_cast<double>(targetResolution.y - 1);

            const double sx0 = u0 * source.GetWidthMinusOne();
            const double sx1 = u1 * source.GetWidthMinusOne();
            const double sy0 = v0 * source.GetHeightMinusOne();
            const double sy1 = v1 * source.GetHeightMinusOne();

            const double sourceVolume = ComputeVolume<false>(source, { { sx0, sy0 }, { sx1, sy1 } });
            const double areaRatio = (targetResolution.x - 1.0) * (targetResolution.y - 1.0) /
                                     (source.GetWidthMinusOne() * source.GetHeightMinusOne());
            referenceNeighborVolumeImage(x, y) = sourceVolume * areaRatio;
        }
    }

    Image<double> referenceCenterVolumeImage(targetResolution.To<uint32_t>());
    for(int y = 0; y < targetResolution.y; ++y)
    {
        for(int x = 0; x < targetResolution.x; ++x)
        {
            const double u0 = (x - 0.5) / static_cast<double>(targetResolution.x - 1);
            const double u1 = (x + 0.5) / static_cast<double>(targetResolution.x - 1);
            const double v0 = (y - 0.5) / static_cast<double>(targetResolution.y - 1);
            const double v1 = (y + 0.5) / static_cast<double>(targetResolution.y - 1);

            const double sx0 = u0 * source.GetWidthMinusOne();
            const double sx1 = u1 * source.GetWidthMinusOne();
            const double sy0 = v0 * source.GetHeightMinusOne();
            const double sy1 = v1 * source.GetHeightMinusOne();

            const double sourceVolume = ComputeVolume<false>(source, { { sx0, sy0 }, { sx1, sy1 } });
            const double areaRatio = (targetResolution.x - 1.0) * (targetResolution.y - 1.0) /
                                     (source.GetWidthMinusOne() * source.GetHeightMinusOne());
            referenceCenterVolumeImage(x, y) = sourceVolume * areaRatio;
        }
    }

    double maxHeight = 0;
    for(double value : source)
    {
        maxHeight = (std::max)(maxHeight, value);
    }

    Image<double> result;
    if(initUsingNativeDownsampler)
    {
        result = Downsample_Native(source, targetResolution);
    }
    else
    {
        result = Image<double>(targetResolution.To<uint32_t>());
    }
    Image<double> tempResult(result.GetSize());

    centerWeight = Saturate(centerWeight);
    const double neighborWeight = 1.0 - centerWeight;
    double lambda = 0.2;
    for(uint32_t i = 0; i < iterations; ++i)
    {
        RTRC_LOG_INFO_SCOPE("Iteration {}/{}", i + 1, iterations);
        ParallelFor(0, targetResolution.y, [&](int y)
        {
            for(int x = 0; x < targetResolution.x; ++x)
            {
                double targetNeighborVolume;
                {
                    const double tx0 = x - 1;
                    const double tx1 = x + 1;
                    const double ty0 = y - 1;
                    const double ty1 = y + 1;
                    targetNeighborVolume = ComputeVolume<true>(result, { { tx0, ty0 }, { tx1, ty1 } });
                }

                const double targetCenterVolume = result(x, y);

                const double referenceNeighborVolume = referenceNeighborVolumeImage(x, y);
                const double referenceCenterVolume = referenceCenterVolumeImage(x, y);

                const double deltaVolume = neighborWeight * (targetNeighborVolume - referenceNeighborVolume) +
                                           4 * centerWeight * (targetCenterVolume - referenceCenterVolume);
                tempResult(x, y) = result(x, y) - lambda * 0.5 * deltaVolume;
            }
        });
        result.Swap(tempResult);
    }

    return result.Map([](const double &f) { return static_cast<double>(f); });
}
