#pragma once

#include <Rtrc/Core/Resource/Image.h>

RTRC_BEGIN

class OT2D
{
public:

    Image<Vector2d> Solve(const Image<double> &sourceDensity) const;
};

RTRC_END
