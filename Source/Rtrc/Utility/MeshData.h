#pragma once

#include <vector>

#include <Rtrc/Math/Vector2.h>
#include <Rtrc/Math/Vector3.h>
#include <Rtrc/Utility/StaticVector.h>

RTRC_BEGIN

class MeshData
{
public:

    std::vector<Vector3f> positionData;
    std::vector<Vector3f> normalData;  // local y
    std::vector<Vector2f> texCoordData;

    // If not present, every triple of vertices constructs a triangle
    std::vector<uint32_t> indexData;

    static MeshData LoadFromObjFile(const std::string &filename);
    
    MeshData RemoveIndexData() const;
};

RTRC_END
