#include <random>

// cy library defines 'restrict' as an macro, which conflicts with mimalloc's declaration when unity build is enabled
#ifdef restrict
#error "restrict macro defined before including cy headers"
#endif
#include <cyVector.h>
#include <cySampleElim.h>
#undef restrict

#include <Rtrc/Core/Math/LowDiscrepancy.h>
#include <Rtrc/Core/Math/Vector.h>

RTRC_BEGIN

namespace LowDiscrepancyDetail
{
    
    template<typename RtrcType, typename CyType, int Channels>
    std::vector<RtrcType> GeneratePoissonDiskSamples(int count, int seed)
    {
        std::default_random_engine rng(seed);
        std::uniform_real_distribution<float> dis(0, 1);

        std::vector<CyType> rawPoints((std::max)(16 * count, 2048));
        for(auto &out : rawPoints)
        {
            for(int i = 0; i < Channels; ++i)
            {
                out[i] = dis(rng);
            }
        }

        std::vector<CyType> outputPoints(count);
        cy::WeightedSampleElimination<CyType, float, Channels> wse;
        wse.SetTiling(true);
        wse.Eliminate(rawPoints.data(), rawPoints.size(), outputPoints.data(), outputPoints.size(), true);

        std::vector<RtrcType> result(count);
        for(int i = 0; i < count; ++i)
        {
            for(int j = 0; j < Channels; ++j)
            {
                result[i][j] = outputPoints[i][j];
            }
        }

        return result;
    }

    template<typename T>
    struct CyTypeTrait { };

    template<>
    struct CyTypeTrait<Vector2f>
    {
        using Type = cy::Vec2f;
        static constexpr int Channels = 2;
    };

    template<>
    struct CyTypeTrait<Vector3f>
    {
        using Type = cy::Vec3f;
        static constexpr int Channels = 3;
    };

    template<>
    struct CyTypeTrait<Vector4f>
    {
        using Type = cy::Vec4f;
        static constexpr int Channels = 4;
    };

} // namespace LowDiscrepancyDetail

template<typename T>
std::vector<T> GeneratePoissonDiskSamples(int count, int seed)
{
    using CyTypeTrait = LowDiscrepancyDetail::CyTypeTrait<T>;
    return LowDiscrepancyDetail::GeneratePoissonDiskSamples
                <T, typename CyTypeTrait::Type, CyTypeTrait::Channels>(count, seed);
}

template std::vector<Vector2f> GeneratePoissonDiskSamples<Vector2f>(int count, int seed);
template std::vector<Vector3f> GeneratePoissonDiskSamples<Vector3f>(int count, int seed);
template std::vector<Vector4f> GeneratePoissonDiskSamples<Vector4f>(int count, int seed);

RTRC_END
