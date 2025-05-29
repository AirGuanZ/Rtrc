#if defined(WIN32) || defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#include <Rtrc/Core/Filesystem/TemporalFile.h>

RTRC_BEGIN

std::string GenerateTemporalFilename(const std::string &prefix)
{
#if defined(WIN32) || defined(_WIN32)
    TCHAR tempPathBuffer[MAX_PATH];
    if(auto len = GetTempPath(MAX_PATH, tempPathBuffer); len == 0 || len > MAX_PATH)
    {
        throw Exception("failed to get temporal path");
    }

    TCHAR tempFileBuffer[MAX_PATH];
    if(GetTempFileName(tempPathBuffer, prefix.c_str(), 0, tempFileBuffer) == 0)
    {
        throw Exception("failed to get temporal file name");
    }

    return tempFileBuffer;
#else
    #error "GenerateTemporalFilename havn't be implemented for non-win32 platform"
#endif
}

RTRC_END
