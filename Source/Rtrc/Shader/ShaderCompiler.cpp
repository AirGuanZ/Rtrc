#include <filesystem>

#include <Rtrc/Shader/DXC.h>
#include <Rtrc/Shader/ResourceDefinitionGenerator.h>
#include <Rtrc/Shader/ShaderCompiler.h>
#include <Rtrc/Utils/Enumerate.h>
#include <Rtrc/Utils/File.h>

RTRC_BEGIN

namespace
{

    class DXCIncludeHandler : public DXC::IncludeHandler
    {
    public:

        bool Handle(const std::string &filename, std::string &output) const override
        {
            try
            {
                output = fileLoader->Load(filename);
                return true;
            }
            catch(...)
            {
                return false;
            }
        }

        RC<FileLoader> fileLoader;
    };

} // namespace anonymous

RHI::Ptr<RHI::RawShader> Shader::GetRawVertexShader() const
{
    return vertexShader_;
}

RHI::Ptr<RHI::RawShader> Shader::GetRawFragmentShader() const
{
    return fragmentShader_;
}

RHI::Ptr<RHI::RawShader> Shader::GetRawComputeShader() const
{
    return computeShader_;
}

std::span<const BindingGroupLayoutInfo*const> Shader::GetBindingGroups() const
{
    return std::span(bindingGroups_.data(), bindingGroups_.size());
}

DefaultFileLoader::DefaultFileLoader(const std::string &rootDir)
    : rootDir_(absolute(std::filesystem::path(rootDir)).lexically_normal())
{

}

std::string DefaultFileLoader::Load(const std::string &filename) const
{
    const std::filesystem::path path = ToStandardFilename(filename);
    return File::ReadTextFile(path.string());
}

std::filesystem::path DefaultFileLoader::ToStandardFilename(const std::string &filename) const
{
    std::filesystem::path path = filename;
    if(path.is_relative())
    {
        path = rootDir_ / path;
    }
    return absolute(path).lexically_normal();
}

ShaderCompiler::ShaderCompiler(RHI::Ptr<RHI::Device> device)
{
    SetDevice(std::move(device));
}

ShaderCompiler& ShaderCompiler::SetDevice(RHI::Ptr<RHI::Device> device)
{
    device_ = std::move(device);
    return *this;
}

ShaderCompiler& ShaderCompiler::SetFileLoader(RC<FileLoader> loader)
{
    fileLoader_ = std::move(loader);
    return *this;
}

ShaderCompiler& ShaderCompiler::SetDefaultFileLoader(std::string rootDir)
{
    fileLoader_ = MakeRC<DefaultFileLoader>(std::move(rootDir));
    return *this;
}

ShaderCompiler& ShaderCompiler::ClearSources()
{
    vsSource_ = {};
    fsSource_ = {};
    csSource_ = {};
    return *this;
}

ShaderCompiler& ShaderCompiler::AddMacro(std::string key, std::string value)
{
    macros_.insert({ std::move(key), std::move(value) });
    return *this;
}

ShaderCompiler& ShaderCompiler::AddBindingGroup(const BindingGroupLayoutInfo* info)
{
    bindingGroups_.push_back(info);
    return *this;
}

ShaderCompiler& ShaderCompiler::SetDebugMode(bool debug)
{
    debugMode_ = debug;
    return *this;
}

ShaderCompiler& ShaderCompiler::DumpPreprocessedSource(RHI::ShaderStage stage, std::string *output)
{
    switch(stage)
    {
    case RHI::ShaderStage::VertexShader:   dumpPreprocessedVS_ = output; break;
    case RHI::ShaderStage::FragmentShader: dumpPreprocessedFS_ = output; break;
    case RHI::ShaderStage::ComputeShader:  dumpPreprocessedCS_ = output; break;
    }
    return *this;
}

RC<Shader> ShaderCompiler::Compile()
{
    if(!fileLoader_)
    {
        SetDefaultFileLoader("./");
    }

    std::string resourceDefinitions;
    std::set<std::string> structNames;
    GenerateResourceDefinitions(resourceDefinitions, structNames);

    auto shader = MakeRC<Shader>();
    if(!vsSource_.filename.empty())
    {
        shader->vertexShader_ = InternalCompileShader(
            vsSource_.filename, vsSource_.entry,
            resourceDefinitions, structNames, RHI::ShaderStage::VertexShader);
    }
    if(!fsSource_.filename.empty())
    {
        shader->fragmentShader_ = InternalCompileShader(
            fsSource_.filename, fsSource_.entry,
            resourceDefinitions, structNames, RHI::ShaderStage::FragmentShader);
    }
    if(!csSource_.filename.empty())
    {
        shader->computeShader_ = InternalCompileShader(
            csSource_.filename, csSource_.entry,
            resourceDefinitions, structNames, RHI::ShaderStage::ComputeShader);
    }

    shader->bindingGroups_ = bindingGroups_;
    return shader;
}

void ShaderCompiler::AddSource(const VSSource& source)
{
    vsSource_ = source;
}

void ShaderCompiler::AddSource(const FSSource& source)
{
    fsSource_ = source;
}

void ShaderCompiler::AddSource(const CSSource& source)
{
    csSource_ = source;
}

void ShaderCompiler::GenerateResourceDefinitions(std::string &definitions, std::set<std::string> &structNames) const
{
    std::string result;
    structNames.clear();
    for(auto &&[setIndex, group] : Enumerate(bindingGroups_))
    {
        auto decl = GenerateHLSLBindingDeclarationForVulkan(structNames, *group, static_cast<int>(setIndex));
        result += decl;
    }
    definitions = result;
}

RHI::Ptr<RHI::RawShader> ShaderCompiler::InternalCompileShader(
    const std::string           &filename,
    const std::string           &entry,
    const std::string           &resourceDefinitions,
    const std::set<std::string> &generatedStructNames,
    RHI::ShaderStage             shaderType) const
{
    DXCIncludeHandler includeHandler;
    includeHandler.fileLoader = fileLoader_;

    DXC::Target target = {};
    switch(shaderType)
    {
    case RHI::ShaderStage::VertexShader:   target = DXC::Target::Vulkan_1_3_VS_6_0; break;
    case RHI::ShaderStage::FragmentShader: target = DXC::Target::Vulkan_1_3_PS_6_0; break;
    case RHI::ShaderStage::ComputeShader:  target = DXC::Target::Vulkan_1_3_CS_6_0; break;
    }

    // preprocess

    const std::string rawSource = fileLoader_->Load(filename);
    std::string preprocessedSource;
    DXC dxc;
    dxc.Compile(DXC::ShaderInfo{
        .source         = rawSource,
        .sourceFilename = filename,
        .entryPoint     = entry,
        .macros         = macros_
    }, target, debugMode_, &includeHandler, &preprocessedSource);

    const std::string finalSource =
        "#line 1 \"RtrcGeneratedResourceDefinitions.hlsl\"\n" + resourceDefinitions + preprocessedSource;

    // dump preprocessed result

    if(dumpPreprocessedVS_ && shaderType == RHI::ShaderStage::VertexShader)
    {
        *dumpPreprocessedVS_ = finalSource;
    }
    else if(dumpPreprocessedFS_ && shaderType == RHI::ShaderStage::FragmentShader)
    {
        *dumpPreprocessedFS_ = finalSource;
    }
    else if(dumpPreprocessedCS_ && shaderType == RHI::ShaderStage::ComputeShader)
    {
        *dumpPreprocessedCS_ = finalSource;
    }

    // compile

    const std::vector<unsigned char> compiledData = dxc.Compile(DXC::ShaderInfo{
        .source         = finalSource,
        .sourceFilename = filename,
        .entryPoint     = entry,
        .macros         = macros_
    }, target, debugMode_, &includeHandler, nullptr);

    return device_->CreateShader(compiledData.data(), compiledData.size(), entry, shaderType);
}

RTRC_END
