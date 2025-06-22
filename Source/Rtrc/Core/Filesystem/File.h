#pragma once

#include <string>
#include <vector>

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

namespace File
{

    std::vector<unsigned char> ReadBinaryFile(const std::string &filename);

    std::string ReadTextFile(const std::string &filename);

    void WriteTextFile(const std::string &filename, std::string_view content);

    uint64_t GetLastWriteTime(const std::string &filename);

} // namespace File

RTRC_END
