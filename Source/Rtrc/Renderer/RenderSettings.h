#pragma once

#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

enum class VisualizationMode
{
    None,
    IndirectDiffuse,
    Normal,
    ReSTIRDirectIllumination,
    Count
};

inline const char *GetVisualizationModeName(VisualizationMode mode)
{
    constexpr const char *ret[] = { "None", "IndirectDiffuse", "Normal", "ReSTIR", "Count" };
    return ret[std::to_underlying(mode)];
}

struct RenderSettings
{
    bool enableRayTracing = true;
    bool enableIndirectDiffuse = false;
    VisualizationMode visualizationMode = VisualizationMode::None;

    unsigned int ReSTIR_M = 4;
    unsigned int ReSTIR_MaxM = 64;
    unsigned int ReSTIR_N = 8;
    float ReSTIR_Radius = 15;
    bool ReSTIR_EnableTemporalReuse = true;
};

RTRC_RENDERER_END
