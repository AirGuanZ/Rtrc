#pragma once

rtrc_group(GlobalBindlessTextureGroup)
{
    rtrc_bindless_varsize(Texture2D<float4>[4096], BindlessTextures)
};
