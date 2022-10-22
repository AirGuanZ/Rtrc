#pragma once

#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Graphics/Object/CopyContext.h>

RTRC_BEGIN

class MeshManager : public Uncopyable
{
public:

    struct Options
    {
        bool generateTangentIfNotPresent = false;
        bool noIndexBuffer = false;
    };

    explicit MeshManager(CopyContext *copyContext);

    Mesh LoadFromObjFile(const std::string &filename, const Options &options = {});

private:
    
    CopyContext *copyContext_;
};

RTRC_END
