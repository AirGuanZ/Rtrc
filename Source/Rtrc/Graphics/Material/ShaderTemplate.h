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

    RC<Shader> GetShader(const KeywordValueContext &keywordValueContext);
    RC<Shader> GetShader(KeywordSet::ValueMask keywordValueMask);

    KeywordSet::ValueMask GetKeywordValueMask(const KeywordValueContext &keywordValueContext) const;

    void GC();

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

    SharedObjectPool<KeywordSet::ValueMask, Shader, true> compiledShaders_;
};

RTRC_END
