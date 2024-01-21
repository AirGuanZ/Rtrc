#pragma once

#include <vector>

#include <Rtrc/Core/Math/Vector2.h>
#include <Rtrc/Core/Math/Vector3.h>

RTRC_BEGIN

class MeshData
{
public:

    std::vector<Vector3f> positionData;
    std::vector<Vector3f> normalData;  // local y
    std::vector<Vector2f> texCoordData;

    // If not present, every triple of vertices constructs a triangle
    std::vector<uint32_t> indexData;

    static MeshData LoadFromObjFile(const std::string &filename, bool wantNormal = true, bool wantTexCoord = true);

    // Merge identical positions and return the mapping from old vertex index to new vertex index
    // normalData and texCoordData are abandoned
    std::vector<size_t> MergeIdenticalPositions();
    
    void RemoveIndexData();
};

RTRC_END
