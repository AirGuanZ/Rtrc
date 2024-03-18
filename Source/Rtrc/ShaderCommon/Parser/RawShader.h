#pragma once

#include <filesystem>
#include <set>

RTRC_BEGIN

struct RawShader
{
    struct SourceRange
    {
        int begin;
        int end;
    };

    std::string shaderName;
    std::string filename; // abs norm path
    std::vector<SourceRange> ranges;
};

struct RawShaderDatabase
{
    std::vector<RawShader> rawShaders;
};

RawShaderDatabase CreateRawShaderDatabase(const std::filesystem::path &filename);

RTRC_END
