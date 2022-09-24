#include <fmt/format.h>

#include <Rtrc/Graphics/Material/ShaderTemplate.h>
#include <Rtrc/Utils/File.h>

RTRC_BEGIN

RC<Shader> ShaderTemplate::GetShader(const KeywordValueContext &keywordValueContext)
{
    const KeywordSet::ValueMask mask = keywordSet_.ExtractValueMask(keywordValueContext);
    return GetShader(mask);
}

RC<Shader> ShaderTemplate::GetShader(KeywordSet::ValueMask keywordValueMask)
{
    return compiledShaders_.GetOrCreate(keywordValueMask, [&]
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
        return shaderCompiler_->Compile(source_, debug_, macros);
    });
}

KeywordSet::ValueMask ShaderTemplate::GetKeywordValueMask(const KeywordValueContext &keywordValueContext) const
{
    return keywordSet_.ExtractValueMask(keywordValueContext);
}

void ShaderTemplate::GC()
{
    compiledShaders_.GC();
}

RTRC_END
