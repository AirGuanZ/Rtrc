#include <ryu/ryu.h>

#include <Rtrc/Core/String.h>

RTRC_BEGIN

std::string ToString(float v)
{
    char buffer[64];
    f2s_buffered(v, buffer);
    return buffer;
}

std::string ToString(double v)
{
    char buffer[64];
    d2s_buffered(v, buffer);
    return buffer;
}

RTRC_END
