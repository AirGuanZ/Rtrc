#pragma once

#include <Rtrc/ToolKit/Resource/ResourceManager.h>

RTRC_RENDERER_BEGIN

RG::TextureResource *Prepare2DPcgStateTexture(
    RG::RenderGraph             &renderGraph,
    Ref<ResourceManager> materials,
    RC<StatefulTexture>         &tex,
    const Vector2u              &size);

RTRC_RENDERER_END
