#pragma once

#include <Rtrc/RHI/RHI.h>

RTRC_BEGIN

class DelayedRHIResourceReleaser
{
public:

    virtual ~DelayedRHIResourceReleaser() = default;

    virtual void DelayedRelease(RHI::Ptr<RHI::RHIObject> ptr) = 0;
};

RTRC_END
