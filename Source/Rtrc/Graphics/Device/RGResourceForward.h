#pragma once

#include <Rtrc/Graphics/Device/AccelerationStructure.h>
#include <Rtrc/Graphics/Device/Buffer/Buffer.h>
#include <Rtrc/Graphics/Device/Texture/Texture.h>

RTRC_RG_BEGIN

class BufferResource;
class TextureResource;
class TlasResource;

RTRC_RG_END

RTRC_BEGIN

RC<Texture> _internalRGGet(RG::TextureResource *tex);
RC<Buffer>  _internalRGGet(RG::BufferResource *buffer);
RC<Tlas>    _internalRGGet(RG::TlasResource *tlas);

RTRC_END
