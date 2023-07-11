#pragma once

#include <Rtrc/Graphics/Device/Device.h>

RTRC_RENDERER_BEGIN

RG::TextureResource *Prepare2DPcgStateTexture(
    RG::RenderGraph             &renderGraph,
    ObserverPtr<MaterialManager> materials,
    RC<StatefulTexture>         &tex,
    const Vector2u              &size);

RTRC_RENDERER_END
