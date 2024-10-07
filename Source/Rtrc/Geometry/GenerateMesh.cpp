#include <numbers>

#include <Rtrc/Geometry/DiscreteCurve.h>
#include <Rtrc/Geometry/GenerateMesh.h>

RTRC_GEO_BEGIN

void GenerateAABB(
    const Vector3d        &lower,
    const Vector3d        &upper,
    std::vector<Vector3d> &outputPositions,
    std::vector<uint32_t> &outputIndices)
{
    constexpr Vector3d LocalPositions[] =
    {
        { 0.5 + +0.500000, 0.5 + -0.500000, 0.5 + +0.500000 },
        { 0.5 + +0.500000, 0.5 + -0.500000, 0.5 + -0.500000 },
        { 0.5 + -0.500000, 0.5 + -0.500000, 0.5 + +0.500000 },
        { 0.5 + -0.500000, 0.5 + -0.500000, 0.5 + -0.500000 },
        { 0.5 + +0.500000, 0.5 + +0.500000, 0.5 + +0.500000 },
        { 0.5 + +0.500000, 0.5 + +0.500000, 0.5 + -0.500000 },
        { 0.5 + -0.500000, 0.5 + +0.500000, 0.5 + +0.500000 },
        { 0.5 + -0.500000, 0.5 + +0.500000, 0.5 + -0.500000 },
    };
    constexpr uint32_t LocalIndices[] =
    {
        4, 2, 0,
        2, 7, 3,
        6, 5, 7,
        1, 7, 5,
        0, 3, 1,
        4, 1, 5,
        4, 6, 2,
        2, 6, 7,
        6, 4, 5,
        1, 3, 7,
        0, 2, 3,
        4, 0, 1,
    };
    const Vector3d size = upper - lower;
    const uint32_t baseIndex = outputPositions.size();
    std::ranges::transform(LocalPositions, std::back_inserter(outputPositions), [&](const Vector3d &localPosition)
    {
        return lower + localPosition * size;
    });
    std::ranges::transform(LocalIndices, std::back_inserter(outputIndices), [&](uint32_t localIndex)
    {
        return baseIndex + localIndex;
    });
}

void GenerateTube(
    Span<Vector3d>         centers,
    bool                   isLoop,
    double                 radius,
    uint32_t               subdivisions,
    bool                   closeEnds,
    std::vector<Vector3d> &outputPositions,
    std::vector<uint32_t> &outputIndices)
{
    assert(subdivisions >= 3);

    std::vector<Vector2d> localPoints;
    localPoints.reserve(subdivisions);
    for(uint32_t i = 0; i < subdivisions; ++i)
    {
        const double alpha = 2.0 * std::numbers::pi * i / subdivisions;
        localPoints.emplace_back(radius * std::cos(alpha), radius * std::sin(alpha));
    }

    const auto frames = ComputeParallelTransportFrame(centers, isLoop);
    const uint32_t baseIndex = outputPositions.size();

    outputPositions.reserve(outputPositions.size() + subdivisions * centers.size());
    for(uint32_t i = 0; i < centers.size(); ++i)
    {
        const Framed &frame = frames[i];
        for(auto &localPoint : localPoints)
        {
            outputPositions.push_back(centers[i] + frame.LocalToGlobal(Vector3d(localPoint, 0.0)));
        }
    }

    auto BuildSegment = [&](uint32_t a, uint32_t b)
    {
        for(uint32_t i = 0; i < subdivisions; ++i)
        {
            const uint32_t j = (i + 1) % subdivisions;

            const uint32_t ai = baseIndex + a * subdivisions + i;
            const uint32_t aj = baseIndex + a * subdivisions + j;
            const uint32_t bi = baseIndex + b * subdivisions + i;
            const uint32_t bj = baseIndex + b * subdivisions + j;

            outputIndices.push_back(ai);
            outputIndices.push_back(bj);
            outputIndices.push_back(bi);

            outputIndices.push_back(ai);
            outputIndices.push_back(aj);
            outputIndices.push_back(bj);
        }
    };
    for(uint32_t i = 1; i < centers.size(); ++i)
    {
        BuildSegment(i - 1, i);
    }
    if(isLoop)
    {
        BuildSegment(centers.size() - 1, 0);
    }

    if(closeEnds && !isLoop)
    {
        const uint32_t iStart = outputPositions.size();
        outputPositions.push_back(centers[0]);
        for(uint32_t i = 0; i < subdivisions; ++i)
        {
            const uint32_t j = (i + 1) % subdivisions;
            outputIndices.push_back(baseIndex + i);
            outputIndices.push_back(iStart);
            outputIndices.push_back(baseIndex + j);
        }

        const uint32_t iEnd = outputPositions.size();
        outputPositions.push_back(centers.last());
        for(uint32_t i = 0; i < subdivisions; ++i)
        {
            const uint32_t j = (i + 1) % subdivisions;
            outputIndices.push_back(baseIndex + (centers.size() - 1) * subdivisions + i);
            outputIndices.push_back(baseIndex + (centers.size() - 1) * subdivisions + j);
            outputIndices.push_back(iEnd);
        }
    }
}

RTRC_GEO_END
