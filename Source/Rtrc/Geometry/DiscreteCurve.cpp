#include <Rtrc/Core/Math/Common.h>
#include <Rtrc/Core/Math/Quaternion.h>
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

std::vector<Framed> ComputeParallelTransportFrame(Span<Vector3d> points, bool isLoop)
{
    const uint32_t N = points.size();

    // Compute tangents

    std::vector<Vector3d> tangents;
    tangents.reserve(N);
    for(uint32_t i = 0; i < N; ++i)
    {
        Vector3d unnormalizedTangent = { 0, 0, 0 };
        if(isLoop)
        {
            const uint32_t iPrev = (N + i - 1) % N;
            const uint32_t iSucc = (i + 1) % N;
            unnormalizedTangent = points[iSucc] - points[iPrev];
        }
        else
        {
            if(i > 0)
            {
                unnormalizedTangent += points[i] - points[i - 1];
            }
            if(i + 1 < N)
            {
                unnormalizedTangent += points[i + 1] - points[i];
            }
        }
        tangents.push_back(NormalizeIfNotZero(unnormalizedTangent));
    }

    // Transport frames

    std::vector<Framed> result;
    result.reserve(N);
    result.push_back(Framed::FromZ(tangents[0]));

    for(uint32_t i = 1; i < N; ++i)
    {
        const Vector3d uA = Cross(tangents[i - 1], tangents[i]);
        if(LengthSquare(uA) < 1e-5) // tangents[i-1] and tangents[i] are (almoat) parallel
        {
            result.push_back(result.back().RotateToNewZ(tangents[i]));
        }
        else
        {
            const Vector3d A = Normalize(uA);
            const double alpha = std::acos(Clamp(Cos(tangents[i - 1], tangents[i]), -1.0, 1.0));
            const Quaterniond rotation = Quaterniond::FromRotationAlongNormalizedAxis(A, alpha);
            const Vector3d newX = rotation.ApplyRotation(result.back().x);
            const Vector3d newY = Cross(tangents[i], newX);
            result.emplace_back(newX, newY, tangents[i]);
        }
    }

    // Spin rotation for closed curve

    if(isLoop)
    {
        LogWarning(
            "Rtrc::Geo::ComputeParallelTransportFrame: "
            "orientation spinning for closed curve hasn't been implemented");
        // TODO
    }

    return result;
}

RTRC_GEO_END
