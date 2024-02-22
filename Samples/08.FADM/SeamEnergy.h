#pragma once

#include "Common.h"

namespace SeamEnergy
{

    Image<Vector4f> ComputePositionMap(const Image<Vector4f> &vdm, const Rectf &gridRect);
    Image<Vector4f> ComputeDisplacementMap(const Image<Vector4f> &posMap, const Rectf &gridRect);
    Image<Vector4f> ComputeNormalMap(const Image<Vector4f> &posMap);

    Image<float> ComputeEnergyMapBasedOnNormalDifference(const Image<Vector4f> &posMap);
    Image<float> ComputeEnergyMapBasedOnInterpolationError(const Image<Vector4f> &posMap, bool interpolateAlongX);

    Image<float> ComputeVerticalDPTable(const Image<float> &energyMap);
    Image<float> ComputeHorizontalDPTable(const Image<float> &energyMap);

    // ret[x] represents coordinate (x, ret[x])
    std::vector<int> GetHorizontalSeam(const Image<float> &dpTable, int entryY);
    std::vector<int> GetVerticalSeam(const Image<float> &dpTable, int entryX);

    Image<Vector4f> RemoveHorizontalSeam(const Image<Vector4f> &posMap, const std::vector<int> &seam);
    Image<Vector4f> RemoveVerticalSeam(const Image<Vector4f> &posMap, const std::vector<int> &seam);

} // namespace SeamEnergy
