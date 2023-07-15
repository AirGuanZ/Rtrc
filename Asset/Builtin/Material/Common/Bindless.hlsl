#pragma once

#include "Constants.hlsl"

#if ENABLE_GLOBAL_BINDLESS_TEXTURES
rtrc_group(GlobalBindlessTextureGroup)
{
    rtrc_bindless_varsize(Texture2D<float4>[GLOBAL_BINDLESS_TEXTURE_GROUP_MAX_SIZE], BindlessTextures)
};
#endif

#if ENABLE_GLOBAL_BINDLESS_GEOMETRY_BUFFERS
rtrc_group(GlobalBindlessGeometryBufferGroup)
{
    rtrc_bindless_varsize(StructuredBuffer<uint>[GLOBAL_BINDLESS_GEOMETRY_BUFFER_MAX_SIZE], BindlessGeometryBuffers)
};
#endif
