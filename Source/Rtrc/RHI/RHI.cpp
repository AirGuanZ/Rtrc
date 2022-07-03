#include <Rtrc/RHI/RHI.h>
#include <Rtrc/Utils/Unreachable.h>

RTRC_RHI_BEGIN

const char *GetFormatName(Format format)
{
#define ADD_CASE(NAME) case Format::NAME: return #NAME;
    switch(format)
    {
    ADD_CASE(Unknown)
    ADD_CASE(B8G8R8A8_UNorm)
    ADD_CASE(R32G32_Float)
    ADD_CASE(R32G32B32A32_Float)
    }
    Unreachable();
#undef ADD_CASE
}

size_t GetTexelSize(Format format)
{
    switch(format)
    {
    case Format::Unknown:            return 0;
    case Format::B8G8R8A8_UNorm:     return 4;
    case Format::R32G32_Float:       return 8;
    case Format::R32G32B32A32_Float: return 16;
    }
    Unreachable();
}

RTRC_RHI_END
