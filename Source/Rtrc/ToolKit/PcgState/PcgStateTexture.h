#pragma once

#include <Rtrc/ToolKit/Resource/ResourceManager.h>

RTRC_BEGIN

RGTexture Prepare2DPcgStateTexture(
    GraphRef             renderGraph,
    RC<StatefulTexture> &tex,
    const Vector2u      &size);

RTRC_END
