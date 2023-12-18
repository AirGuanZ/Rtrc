#pragma once

#include <ankerl/unordered_dense.h>

#include <Graphics/Shader/Compiler.h>
#include <Graphics/Shader/Keyword.h>

RTRC_BEGIN

class ShaderDatabase;

void RegisterAllPreprocessedShadersInShaderDatabase(ShaderDatabase &database);

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

        RC<Shader> GetVariant(bool persistent = true);
        RC<Shader> GetVariant(const FastKeywordSetValue &keywords, bool persistent = true);
        RC<Shader> GetVariant(const FastKeywordContext &keywords, bool persistent = true);
        const FastKeywordSet &GetKeywordSet() const;
        bool HasBuiltinKeyword(BuiltinKeyword keyword) const;

    private:

        friend class ShaderDatabase;

        ShaderDatabase *database_ = nullptr;
        ShaderRecord *record_ = nullptr;
        
        std::vector<RC<Shader>> persistentShaders;
    };

    ShaderDatabase();

    void SetDevice(Ref<Device> device);
    void SetDebug(bool debug);
    void AddIncludeDirectory(std::string dir);
    
    void AddShader(ParsedShader shader);

    RC<ShaderTemplate> GetShaderTemplate(GeneralPooledString name, bool persistent);
    RC<ShaderTemplate> GetShaderTemplate(std::string_view name, bool persistent);
    
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
        ParsedShader        parsedShader;
        bool                hasBuiltinKeyword[EnumCount<BuiltinKeyword>];

        tbb::spin_rw_mutex shaderTemplateMutex;
        RC<ShaderTemplate> shaderTemplate;
    };

    RC<Shader> GetShaderImpl(ShaderRecord *record, FastKeywordSetValue fastKeywordValues);

    ObjectCache<ShaderKey, Shader, true, true>                           shaders_;
    ankerl::unordered_dense::map<GeneralPooledString, Box<ShaderRecord>> records_;

    bool                     debug_ = RTRC_DEBUG;
    ShaderCompiler                 compiler_;
    ShaderCompileEnvironment envir_;
};

using ShaderTemplate = ShaderDatabase::ShaderTemplate;

RTRC_END
