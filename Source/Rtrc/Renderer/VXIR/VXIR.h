#pragma once

#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

namespace VXIRDetail
{

    rtrc_struct(HashTableKey)
    {
        uint key;
    };

    rtrc_struct(HashTableValue)
    {
        float3 position;
        float3 normal;

        float3 irradiance;
        uint   valid;

        float3 fullWeightedSum;
        float  fullWeight;
        float3 halfWeightedSum;
        float  halfWeight;
    };

} // namespace VXIRDetail

/*
    1. Update existing hash table items
        * Accumulate new radiance samples onto sum/weight
        * Insert new items
    2. Final gather with bn mask & ReSTIR
        * Insert new items
*/
class VXIR : public RenderAlgorithm
{
public:

    struct Settings
    {
        float    maxDist;
        float    minVoxelSize;
        float    maxVoxelSize;
        uint32_t hashTableSize;
    };

    struct PerCameraData
    {
        RC<StatefulBuffer> hashTableKeys;
        RC<StatefulBuffer> hashTableValues;
    };

    using RenderAlgorithm::RenderAlgorithm;

    RTRC_SET_GET(Settings, Settings, settings_)

    void Render(
        ObserverPtr<RG::RenderGraph> renderGraph,
        ObserverPtr<RenderCamera>    renderCamera) const;

    void ClearFrameData(PerCameraData &data) const;

private:

    void PrepareHashTable(ObserverPtr<RG::RenderGraph> renderGraph, PerCameraData &data) const;

    Settings settings_;
};

RTRC_RENDERER_END
