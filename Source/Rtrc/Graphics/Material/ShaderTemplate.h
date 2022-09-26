#pragma once

#include <Rtrc/Graphics/Material/Keyword.h>
#include <Rtrc/Graphics/Shader/ShaderCompiler.h>

RTRC_BEGIN

// Keywords -> Shader
class ShaderTemplate : public Uncopyable
{
public:

    struct SharedCompileEnvironment
    {
        std::map<std::string, std::string> macros;
    };

    RC<Shader> GetShader(const KeywordValueContext &keywordValueContext)
    {
        const KeywordSet::ValueMask mask = keywordSet_.ExtractValueMask(keywordValueContext);
        return GetShader(mask);
    }

    RC<Shader> GetShader(KeywordSet::ValueMask keywordValueMask)
    {
        assert(compiledShaders_.size() > keywordValueMask);
        if(!compiledShaders_[keywordValueMask])
        {
            ShaderCompiler::Macros macros = sharedEnvir_->macros;
            keywordSet_.ForEachKeywordAndValue(keywordValueMask, [&](const Keyword &keyword, uint8_t value)
            {
                macros[keyword.GetString()] = std::to_string(value);
            });
            if(debug_)
            {
                macros["ENABLE_SHADER_DEBUG"] = "1";
            }
            compiledShaders_[keywordValueMask] = shaderCompiler_->Compile(source_, debug_, macros);
        }
        return compiledShaders_[keywordValueMask];
    }

    const KeywordSet &GetKeywordSet() const { return keywordSet_; }
    KeywordSet::ValueMask GetKeywordValueMask(const KeywordValueContext &keywordValueContext) const
    {
        return keywordSet_.ExtractValueMask(keywordValueContext);
    }

private:

    friend class MaterialManager;

    // Necessary info for compilation

    bool debug_ = RTRC_DEBUG;
    KeywordSet keywordSet_;
    ShaderCompiler::ShaderSource source_;
    RC<SharedCompileEnvironment> sharedEnvir_;

    // External compiler

    ShaderCompiler *shaderCompiler_ = nullptr;

    // Result

    std::vector<RC<Shader>> compiledShaders_;
};

RTRC_END
