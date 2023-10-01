#pragma once

#include <filesystem>

#include <ShaderCompiler/Parser/BindingGroupParser.h>

RTRC_SHADER_COMPILER_BEGIN

struct RawPassRecord
{
    std::string name;
    size_t      charBegin; // Relative to material.charBegin
    size_t      charEnd;
};

struct RawMaterialProperty
{
    std::string type;
    std::string name;
};

struct RawMaterialRecord
{
    std::string                      name;
    size_t                           charBegin;
    size_t                           charEnd;
    std::vector<RawMaterialProperty> properties;
    std::vector<RawPassRecord>       passes;
};

struct RawShader
{
    std::string shaderName;
    std::string filename; // abs norm path
    int         charBegin;
    int         charEnd;
};

struct RawShaderDatabase
{
    std::vector<RawShader> rawShaders;
};

/*
    rtrc_material("MaterialName")
        rtrc_pass("PassName")
            rtrc_shader("ShaderName")
            ...
        rtrc_pass("PassName")
            rtrc_shader_ref("ShaderName")
*/
std::vector<RawMaterialRecord> ParseRawMaterials(const std::string &rawSource);

RawShaderDatabase CreateRawShaderDatabase(const std::set<std::filesystem::path> &filenames);

RTRC_SHADER_COMPILER_END
