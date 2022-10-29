#pragma once

#include <vector>

#include <Rtrc/Graphics/RHI/RHI.h>

RTRC_BEGIN

namespace ShaderPreprocess
{

    bool ParseEntryPragma(std::string &source, const std::string &entryName, std::string &result);

    void ParseKeywords(std::string &source, std::vector<std::string> &keywords);

    void RemoveComments(std::string &source);

} // namespace ShaderPreprocess

RTRC_END
