#pragma once

#include <Graphics/Device/Device.h>
#include <Rtrc/Resource/ResourceManager.h>

RTRC_BEGIN

// Implementation of distortion-free displacement mapping optimizer
// See https://cg.ivd.kit.edu/publications/2019/undistort/distortion-free-displacement-preprint.pdf
class DFDM
{
public:

    enum class WrapMode
    {
        Clamp,
        Repeat
    };

    RTRC_SET_GET(ObserverPtr<ResourceManager>, Resources,      resources_)
    RTRC_SET_GET(WrapMode,                     WrapMode,       wrapMode_)
    RTRC_SET_GET(int,                          IterationCount, iterationCount_)

    Image<Vector2f> GenerateCorrectionMap(const Image<Vector3f>& displacementMap) const;

private:

    ObserverPtr<ResourceManager> resources_;
    WrapMode                     wrapMode_       = WrapMode::Clamp;
    int                          iterationCount_ = 300;
};

RTRC_END
