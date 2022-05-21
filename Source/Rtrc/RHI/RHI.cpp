#include <Rtrc/RHI/RHI.h>
#include <Rtrc/Utils/Unreachable.h>

RTRC_RHI_BEGIN

const char *GetTexelFormatName(TexelFormat format)
{
#define ADD_CASE(NAME) case TexelFormat::NAME: return #NAME;
    switch(format)
    {
    ADD_CASE(B8G8R8A8_UNorm)
    }
    Unreachable();
#undef  ADD_CASE
}

RTRC_RHI_END
