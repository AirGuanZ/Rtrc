#include <Rtrc/Graphics/Device/RGResourceForward.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_BEGIN

RC<Texture> _internalRGGet(const RG::TextureResource *tex)
{
    return tex->Get();
}

RC<Buffer> _internalRGGet(const RG::BufferResource *buffer)
{
    return buffer->Get();
}

RC<Tlas> _internalRGGet(const RG::TlasResource *tlas)
{
    return tlas->Get();
}

RTRC_END
