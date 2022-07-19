#pragma once

#include <map>
#include <span>

#include <Rtrc/Shader/Binding.h>

RTRC_BEGIN

class ShaderCompileResult : public Uncopyable
{
public:

    RHI::Ptr<RHI::RawShader> GetRawVertexShader() const;

    RHI::Ptr<RHI::RawShader> GetRawFragmentShader() const;

    RHI::Ptr<RHI::RawShader> GetRawComputeShader() const;

    std::span<const BindingGroupLayoutInfo * const> GetBindingGroups() const;

private:

    friend class ShaderCompiler;

    RHI::Ptr<RHI::RawShader> vertexShader_;
    RHI::Ptr<RHI::RawShader> fragmentShader_;
    RHI::Ptr<RHI::RawShader> computeShader_;

    std::vector<const BindingGroupLayoutInfo *> bindingGroups_;
};

struct VSSource
{
    std::string filename;
    std::string entry;
};

struct FSSource
{
    std::string filename;
    std::string entry;
};

struct CSSource
{
    std::string filename;
    std::string entry;
};

template<typename T>
constexpr bool IsShaderSource = std::is_same_v<T, VSSource> ||
                                std::is_same_v<T, FSSource> ||
                                std::is_same_v<T, CSSource>;

class FileLoader
{
public:

    virtual ~FileLoader() = default;

    virtual std::string Load(const std::string &filename) const = 0;
};

class DefaultFileLoader : public FileLoader
{
public:

    explicit DefaultFileLoader(const std::string &rootDir);

    std::string Load(const std::string &filename) const override;

protected:

    std::filesystem::path ToStandardFilename(const std::string &filename) const;

    std::filesystem::path rootDir_;
};

// TODO: replace this with ShaderManager
class ShaderCompiler : public Uncopyable
{
public:

    explicit ShaderCompiler(RHI::Ptr<RHI::Device> device = {});

    ShaderCompiler &SetDevice(RHI::Ptr<RHI::Device> device);

    ShaderCompiler &SetFileLoader(RC<FileLoader> loader);

    ShaderCompiler &SetDefaultFileLoader(std::string rootDir);

    ShaderCompiler &ClearSources();

    template<typename...Ts> requires (IsShaderSource<Ts> && ...)
    ShaderCompiler &AddSources(const Ts&...sources);

    ShaderCompiler &AddMacro(std::string key, std::string value);

    ShaderCompiler &AddBindingGroup(const BindingGroupLayoutInfo *info);

    template<BindingGroupStruct T>
    ShaderCompiler &AddBindingGroup();

    ShaderCompiler &SetDebugMode(bool debug);

    ShaderCompiler &DumpPreprocessedSource(RHI::ShaderStage stage, std::string *output);

    RC<ShaderCompileResult> Compile();

private:

    void AddSource(const VSSource &source);

    void AddSource(const FSSource &source);

    void AddSource(const CSSource &source);

    void GenerateResourceDefinitions(std::string &definitions, std::set<std::string> &structNames) const;

    RHI::Ptr<RHI::RawShader> InternalCompileShader(
        const std::string           &filename,
        const std::string           &entry,
        const std::string           &resourceDefinitions,
        const std::set<std::string> &generatedStructNames,
        RHI::ShaderStage             shaderType) const;

    RHI::Ptr<RHI::Device> device_;
    RC<FileLoader> fileLoader_;

    VSSource vsSource_;
    FSSource fsSource_;
    CSSource csSource_;
    
    std::string *dumpPreprocessedVS_ = nullptr;
    std::string *dumpPreprocessedFS_ = nullptr;
    std::string *dumpPreprocessedCS_ = nullptr;

    std::map<std::string, std::string> macros_;
    std::vector<const BindingGroupLayoutInfo *> bindingGroups_;
    
    bool debugMode_ = RTRC_DEBUG == 1;
};

template<typename ... Ts> requires (IsShaderSource<Ts> && ...)
ShaderCompiler& ShaderCompiler::AddSources(const Ts&...sources)
{
    (this->AddSource(sources), ...);
    return *this;
}

template<BindingGroupStruct T>
ShaderCompiler &ShaderCompiler::AddBindingGroup()
{
    return this->AddBindingGroup(GetBindingGroupLayoutInfo<T>());
}

RTRC_END
