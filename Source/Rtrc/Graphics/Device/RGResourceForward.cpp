#include <Rtrc/Graphics/Device/RGResourceForward.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_BEGIN

RC<Texture> _internalRGGet(RG::TextureResource *tex)
{
    return tex->Get();
}

RC<Buffer> _internalRGGet(RG::BufferResource *buffer)
{
    return buffer->Get();
}

RC<Tlas> _internalRGGet(RG::TlasResource *tlas)
{
    return tlas->Get();
}

RTRC_END
