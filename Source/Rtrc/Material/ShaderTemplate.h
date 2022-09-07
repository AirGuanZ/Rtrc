#pragma once

#include <Rtrc/Material/Keyword.h>
#include <Rtrc/Shader/ShaderCompiler.h>

RTRC_BEGIN

// Keywords -> Shader
class ShaderTemplate : public Uncopyable
{
public:

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

    // External compiler

    ShaderCompiler *shaderCompiler_ = nullptr;

    // Result

    SharedObjectPool<KeywordSet::ValueMask, Shader, true> compiledShaders_; // TODO IMPROVE: keyword trie
};

RTRC_END
