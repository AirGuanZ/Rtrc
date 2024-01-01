#pragma once

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/ToolKit/Resource/ResourceManager.h>

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

    RTRC_SET_GET(Ref<ResourceManager>, Resources,      resources_)
    RTRC_SET_GET(WrapMode,                     WrapMode,       wrapMode_)
    RTRC_SET_GET(int,                          IterationCount, iterationCount_)

    Image<Vector2f> GenerateCorrectionMap(const Image<Vector3f>& displacementMap) const;

private:

    Ref<ResourceManager> resources_;
    WrapMode             wrapMode_       = WrapMode::Clamp;
    int                  iterationCount_ = 300;
};

RTRC_END
