#include <Rtrc/Core/Math/Common.h>
#include <Rtrc/Geometry/DiscreteCurve.h>

RTRC_GEO_BEGIN

namespace DiscreteCurveDetail
{

    template<typename Func>
    void ForEachSegment(uint32_t numPoints, bool isLoop, const Func &func)
    {
        assert(numPoints >= 2 && "A discrete curve should contain at least 2 points");
        assert((!isLoop || numPonits >= 3) && "A closed discrete curve should contain at least 3 points");
        for(uint32_t i = 1; i < numPoints; ++i)
        {
            func(i - 1, i);
        }
        if(isLoop)
        {
            func(numPoints - 1, 0);
        }
    }

} // namespace DiscreteCurveDetail

double ComputeDiscreteCurveLength(Span<Vector3d> points, bool isLoop)
{
    double length = 0;
    DiscreteCurveDetail::ForEachSegment(points.size(), isLoop, [&](uint32_t ia, uint32_t ib)
    {
        length += Distance(points[ia], points[ib]);
    });
    return length;
}

std::vector<Vector3d> UniformSampleDiscreteCurve(Span<Vector3d> points, bool isLoop, uint32_t numSamples)
{
    assert(numSamples >= 2);
    const double totalLength = ComputeDiscreteCurveLength(points, isLoop);
    const uint32_t numSegments = isLoop ? numSamples : (numSamples - 1);
    const double segmentLength = totalLength / numSegments;

    std::vector<Vector3d> result;
    double accumulatedLength = 0;
    DiscreteCurveDetail::ForEachSegment(points.size(), isLoop, [&](uint32_t ia, uint32_t ib)
    {
        const Vector3d &a = points[ia];
        const Vector3d &b = points[ib];
        const double L = Distance(a, b);
        double t = 0;
        while(true)
        {
            const double r = segmentLength - accumulatedLength;
            if(t + r <= L)
            {
                t += r;
                accumulatedLength = 0;

                const uint32_t limit = isLoop ? numSamples : (numSamples - 1);
                if(result.size() < limit)
                {
                    result.push_back(Lerp(a, b, t / L));
                }
            }
            else
            {
                accumulatedLength += L - t;
                break;
            }
        }
    });

    if(!isLoop)
    {
        assert(result.size() == numSamples - 1);
        result.push_back(points.last());
    }
    else
    {
        assert(result.size() == numSamples);
    }
    return result;
}

RTRC_GEO_END
