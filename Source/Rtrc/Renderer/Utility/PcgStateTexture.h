#pragma once

#include <Graphics/Device/Device.h>
#include <Graphics/RenderGraph/Graph.h>
#include <Rtrc/Resource/ResourceManager.h>

RTRC_RENDERER_BEGIN

RG::TextureResource *Prepare2DPcgStateTexture(
    RG::RenderGraph             &renderGraph,
    ObserverPtr<ResourceManager> materials,
    RC<StatefulTexture>         &tex,
    const Vector2u              &size);

RTRC_RENDERER_END
