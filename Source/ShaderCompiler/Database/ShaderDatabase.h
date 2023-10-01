#pragma once

#include <ankerl/unordered_dense.h>

#include <ShaderCompiler/Compiler/Compiler.h>
#include <ShaderCompiler/Database/Keyword.h>

#include "ShaderDatabase.h"

RTRC_SHADER_COMPILER_BEGIN

class ShaderDatabase;

class ShaderDatabase
{
    struct ShaderRecord;

public:

    struct ShaderVariant
    {
        CompilableShader compilableShader;
        RC<Shader>       shader;
    };

    class ShaderTemplate
    {
    public:

        RC<Shader> GetDefaultVariant();
        RC<Shader> GetVariant(const FastKeywordSetValue &keywords);
        RC<Shader> GetVariant(const FastKeywordContext &keywords);
        const FastKeywordSet &GetKeywordSet() const;

    private:

        friend class ShaderDatabase;

        ShaderDatabase *database_ = nullptr;
        const ShaderRecord *record_ = nullptr;
    };

    void SetDevice(ObserverPtr<Device> device);
    void SetDebug(bool debug);
    void AddIncludeDirectory(std::string dir);
    
    void AddShader(ParsedShader shader);

    const ShaderTemplate *GetShaderTemplate(GeneralPooledString name);
    const ShaderTemplate *GetShaderTemplate(std::string_view name);

    RC<Shader> GetShader(GeneralPooledString name, const FastKeywordSetValue &keywords);
    RC<Shader> GetShader(GeneralPooledString name, const FastKeywordContext &keywords);

private:

    struct ShaderKey
    {
        uint64_t keyValue = 0;

        ShaderKey() = default;
        ShaderKey(GeneralPooledString name, FastKeywordSetValue keywords);
        auto operator<=>(const ShaderKey &) const = default;
        size_t Hash() const { return Rtrc::Hash(keyValue); }
    };

    struct ShaderRecord
    {
        GeneralPooledString name;
        FastKeywordSet      keywordSet;
        Box<ShaderTemplate> shaderTemplate;
        ParsedShader        parsedShader;
    };

    RC<Shader> GetShaderImpl(const ShaderRecord *record, FastKeywordSetValue fastKeywordValues);

    ankerl::unordered_dense::map<GeneralPooledString, Box<ShaderRecord>> records_;
    ObjectCache<ShaderKey, Shader, true, true>                           shaders_;

    bool                     debug_ = RTRC_DEBUG;
    Compiler                 compiler_;
    ShaderCompileEnvironment envir_;
};

RTRC_SHADER_COMPILER_END
