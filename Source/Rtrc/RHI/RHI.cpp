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
    }
    Unreachable();
#undef ADD_CASE
}

RTRC_RHI_END
