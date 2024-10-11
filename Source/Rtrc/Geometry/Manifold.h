#pragma once

#include <Rtrc/Geometry/HalfedgeMesh.h>

RTRC_GEO_BEGIN

// See SplitNonManifoldInput flag in HalfedgeMesh
void ResolveNonManifoldConnectivity(std::vector<uint32_t> &indices, std::vector<uint32_t> &newVertexToOldVertex);

bool IsOrientable(const HalfedgeMesh &connectivity);

RTRC_GEO_END
