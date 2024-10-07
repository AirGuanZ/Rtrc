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
    int                    segments,
    std::vector<Vector3d> &outputPositions,
    std::vector<uint32_t> &outputIndices)
{
    const auto frames = ComputeParallelTransportFrame(centers, isLoop);

}

RTRC_GEO_END
