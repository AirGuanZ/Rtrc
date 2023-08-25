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

RC<Texture> _internalRGGet(const RG::TextureResource *tex);
RC<Buffer>  _internalRGGet(const RG::BufferResource *buffer);
RC<Tlas>    _internalRGGet(const RG::TlasResource *tlas);

RTRC_END
