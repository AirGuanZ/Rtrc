#include <random>

#include <cyVector.h>
#include <cySampleElim.h>

#include <Rtrc/Math/LowDiscrepancy.h>

RTRC_BEGIN

std::vector<Vector2f> GeneratePoissonDiskSamples(int count, int seed)
{
    std::default_random_engine rng(seed);
    std::uniform_real_distribution<float> dis(0, 1);

    std::vector<cy::Vec2f> rawPoints(16 * count);
    for(size_t i = 0; i < rawPoints.size(); ++i)
    {
        const float u = dis(rng);
        const float v = dis(rng);
        rawPoints[i] = { u, v };
    }

    std::vector<cy::Vec2f> outputPoints(count);
    cy::WeightedSampleElimination<cy::Vec2f, float, 2> wse;
    wse.SetTiling(true);
    wse.Eliminate(rawPoints.data(), rawPoints.size(), outputPoints.data(), outputPoints.size());

    std::vector<Vector2f> result(count);
    for(int i = 0; i < count; ++i)
    {
        result[i].x = outputPoints[i].x;
        result[i].y = outputPoints[i].y;
    }
    return result;
}

RTRC_END
