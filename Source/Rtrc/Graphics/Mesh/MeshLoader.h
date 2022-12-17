#pragma once

#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Graphics/Device/CopyContext.h>

RTRC_BEGIN

class MeshLoader : public Uncopyable
{
public:

    struct Options
    {
        bool generateTangentIfNotPresent = false;
        bool noIndexBuffer = false;
    };

    void SetCopyContext(CopyContext *copyContext);

    void SetRootDirectory(std::string_view rootDir);

    Mesh LoadFromObjFile(const std::string &filename, const Options &options = {});

private:

    std::string MapFilename(std::string_view filename) const;

    std::filesystem::path rootDir_;
    CopyContext *copyContext_ = nullptr;
};

RTRC_END
