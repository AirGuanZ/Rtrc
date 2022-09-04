#pragma once

#include <Rtrc/Material/Keyword.h>
#include <Rtrc/Shader/ShaderBindingParser.h>
#include <Rtrc/Shader/ShaderCompiler.h>

RTRC_BEGIN

class ShaderManager;

// Keywords -> Shader
class ShaderTemplate : public Uncopyable
{
public:

    RC<Shader> GetShader(const KeywordValues &keywordMap);

    void GC();

private:

    friend class ShaderManager;

    // Necessary info for compilation

    bool debug_ = RTRC_DEBUG;
    Keywords keywords_;
    ShaderCompiler::ShaderSource source_;

    // External compiler

    ShaderCompiler *shaderCompiler_ = nullptr;

    // Result

    SharedObjectPool<KeywordValues, Shader, true> compiledShaders_;
};

class ShaderManager : public Uncopyable
{
public:

    void SetDevice(RHI::DevicePtr device);
    void SetRootDirectory(std::string_view rootDir);

    RC<ShaderTemplate> GetShaderTemplate(std::string_view name) const;

    void AddShaderTemplate(
        const std::string &name,
        const std::string &source,
        const std::string &filename,
        bool               debug = RTRC_DEBUG);

    void AddShaderFile(const std::string &filename, bool debug = RTRC_DEBUG);

private:

    std::string rootDirectory_;

    ShaderCompiler shaderCompiler_;

    std::map<std::string, RC<ShaderTemplate>, std::less<>> nameToTemplate_;
    mutable std::mutex mutex_;
};

RTRC_END
