#pragma once

#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

enum class VisualizationMode
{
    None,
    IndirectDiffuse,
    Count
};

inline const char *GetVisualizationModeName(VisualizationMode mode)
{
    constexpr const char *ret[] = { "None", "IndirectDiffuse", "Count" };
    return ret[std::to_underlying(mode)];
}

struct RenderSettings
{
    VisualizationMode visualizationMode = VisualizationMode::None;
};

RTRC_RENDERER_END