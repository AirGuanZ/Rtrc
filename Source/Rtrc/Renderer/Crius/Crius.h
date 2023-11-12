#pragma once

#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

namespace CriusDetail
{

    rtrc_enum(SurfelCounter)
    {
        SC_ActiveSurfelCount,
        SC_Count
    };

    rtrc_struct(Surfel)
    {
        float3 position;
        float3 normal;
        float3 irradiance;
        float2 moments;
    };

} // namespace CriusDetail

class Crius : public RenderAlgorithm
{
public:

    struct Settings
    {
        int maxSurfelCount;
    };

    struct PerCameraData
    {
        RC<StatefulBuffer> surfelBuffer;
        RC<StatefulBuffer> surfelCounterBuffer;
    };

    using RenderAlgorithm::RenderAlgorithm;

    RTRC_SET_GET(Settings, Settings, settings_)

    void Render(
        ObserverPtr<RG::RenderGraph> renderGraph,
        ObserverPtr<RenderCamera>    renderCamera) const;

private:

    Settings settings_;
};

RTRC_RENDERER_END
