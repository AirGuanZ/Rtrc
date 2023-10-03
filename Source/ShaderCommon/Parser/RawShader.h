#pragma once

#include <filesystem>

#include <Core/Serialization/Serialize.h>
#include <ShaderCommon/Parser/BindingGroupParser.h>

RTRC_BEGIN

struct RawPassRecord
{
    std::string name;
    std::string shaderName;
    std::vector<std::string> tags;

    size_t charBegin = 0; // Relative to material.charBegin
    size_t charEnd   = 0;

    RTRC_AUTO_SERIALIZE(name, shaderName, tags);
};

struct RawMaterialProperty
{
    std::string type;
    std::string name;

    RTRC_AUTO_SERIALIZE(type, name);
};

struct RawMaterialRecord
{
    std::string                      name;
    size_t                           charBegin = 0;
    size_t                           charEnd   = 0;
    std::vector<RawMaterialProperty> properties;
    std::vector<RawPassRecord>       passes;

    RTRC_AUTO_SERIALIZE(name, properties, passes);
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
    {
        rtrc_pass("PassName")
        {
            rtrc_shader("ShaderName")
            {
                ...
            }
        }
        rtrc_pass("AnotherPassName")
        {
            rtrc_shader_ref("AnotherShaderName")
        }
    }
    rtrc_shader("AnotherShaderName")
    {
        ...
    }
*/
void ParseRawMaterials(
    const std::string              &rawSource,
    std::vector<RawMaterialRecord> &rawMaterials,
    std::vector<RawShader>         &rawShaders); // Note that rawShader.filename is not filled

RawShaderDatabase CreateRawShaderDatabase(const std::set<std::filesystem::path> &filenames);

RTRC_END
