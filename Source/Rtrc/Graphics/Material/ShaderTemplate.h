#pragma once

#include <Rtrc/Graphics/Material/Keyword.h>
#include <Rtrc/Graphics/Shader/ShaderCompiler.h>
#include <Rtrc/Utility/Container/ObjectCache.h>

RTRC_BEGIN

// Keywords -> Shader
class ShaderTemplate : public InObjectCache
{
public:

    struct SharedCompileEnvironment
    {
        std::map<std::string, std::string> macros;
    };

    bool HasBuiltinKeyword(BuiltinKeyword keyword) const
    {
        return builtinKeywordMask_.test(std::to_underlying(keyword));
    }

    RC<Shader> GetShader(const KeywordContext &keywordValueContext)
    {
        const KeywordSet::ValueMask mask = keywordSet_.ExtractValueMask(keywordValueContext);
        return GetShader(mask);
    }

    RC<Shader> GetShader(KeywordSet::ValueMask keywordValueMask = 0)
    {
        std::lock_guard lock(compiledShadersMutex_); // TODO: optimize for parallel reads
        assert(compiledShaders_.size() > keywordValueMask);
        if(!compiledShaders_[keywordValueMask])
        {
            ShaderCompiler::Macros macros;
            if(sharedEnvir_)
            {
                macros = sharedEnvir_->macros;
            }
            keywordSet_.ForEachKeywordAndValue(keywordValueMask, [&](const Keyword &keyword, uint8_t value)
            {
                macros[keyword.GetString()] = std::to_string(value);
            });
            if(debug_)
            {
                macros["ENABLE_SHADER_DEBUG"] = "1";
            }
            compiledShaders_[keywordValueMask] = shaderCompiler_->Compile(source_, macros, debug_);
        }
        return compiledShaders_[keywordValueMask];
    }

    const KeywordSet &GetKeywordSet() const { return keywordSet_; }
    KeywordSet::ValueMask GetKeywordValueMask(const KeywordContext &keywordValueContext) const
    {
        return keywordSet_.ExtractValueMask(keywordValueContext);
    }

private:

    friend class MaterialManager;

    std::bitset<EnumCount<BuiltinKeyword>> builtinKeywordMask_;

    // Necessary info for compilation

    bool                         debug_ = RTRC_DEBUG;
    KeywordSet                   keywordSet_;
    ShaderCompiler::ShaderSource source_;
    RC<SharedCompileEnvironment> sharedEnvir_;

    // External compiler

    ShaderCompiler *shaderCompiler_ = nullptr;

    // Result

    std::mutex compiledShadersMutex_;
    std::vector<RC<Shader>> compiledShaders_;
};

RTRC_END
