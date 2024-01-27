#pragma once

#include <Rtrc/Core/Common.h>

#if RTRC_RHI_DIRECTX12
#define RTRC_PIX_CAPTURER 1
#endif

RTRC_RHI_BEGIN

class GPUCapturer
{
public:

    virtual ~GPUCapturer() = default;

    virtual bool Begin(const std::string &outputFilename) = 0;

    virtual bool End() = 0;
};

#if RTRC_PIX_CAPTURER
Box<GPUCapturer> CreatePIXCapturer();
#endif

RTRC_RHI_END
