#pragma once

#include <filesystem>
#include <map>

#include <Rtrc/Shader/ShaderBindingGroup.h>
#include <Rtrc/Shader/ShaderReflection.h>
#include <Rtrc/Utils/SharedObjectPool.h>

RTRC_BEGIN

class Shader : public Uncopyable
{
public:

    const RHI::RawShaderPtr &GetRawShader(RHI::ShaderStage stage) const;
    const RHI::BindingLayoutPtr &GetRHIBindingLayout() const;

    Span<ShaderIOVar> GetInputVariables() const;

    const RC<BindingGroupLayout> GetBindingGroupLayoutByName(std::string_view name) const;
    const RC<BindingGroupLayout> GetBindingGroupLayoutByIndex(int index) const;
    int GetBindingGroupIndexByName(std::string_view name) const;

private:

    friend class ShaderCompiler;

    RHI::RawShaderPtr VS_;
    RHI::RawShaderPtr FS_;
    RHI::RawShaderPtr CS_;

    ShaderReflection VSRefl_;
    ShaderReflection FSRefl_;
    ShaderReflection CSRefl_;

    std::map<std::string, int, std::less<>> nameToBindingGroupLayoutIndex_;
    std::vector<RC<BindingGroupLayout>>     bindingGroupLayouts_;
    RC<RHI::BindingLayoutPtr>               rhiBindingLayout_;
};

class ShaderCompiler : public Uncopyable
{
public:

    using Macros = std::map<std::string, std::string>;

    struct ShaderSource
    {
        std::string filename;
        std::string source;

        std::string VSEntry;
        std::string FSEntry;
        std::string CSEntry;
    };

    void SetDevice(RHI::DevicePtr device);

    void SetRootDirectory(std::string_view rootDir);

    RC<Shader> Compile(
        const ShaderSource &desc,
        bool                debug = RTRC_DEBUG,
        const Macros       &macros = {},
        bool                skipPreprocess = false);

    std::string Preprocess(std::string source, std::string filename, const Macros &macros);

    std::string GetMappedFilename(const std::string &filename);

private:

    RHI::RawShaderPtr CompileShader(
        const std::string                               &source,
        const std::string                               &filename,
        const std::string                               &entry,
        bool                                             debug,
        const std::map<std::string, std::string>        &macros,
        RHI::ShaderStage                                 stage,
        bool                                             skipPreprocess,
        std::map<std::string, int, std::less<>>         &outputNameToGroupIndex,
        std::vector<RC<BindingGroupLayout>>             &outBindingGroupLayouts,
        ShaderReflection                                &outputRefl);

    RHI::DevicePtr rhiDevice_;
    std::filesystem::path rootDir_;

    SharedObjectPool<RHI::BindingGroupLayoutDesc, BindingGroupLayout, true> bindingGroupLayoutPool_;
    SharedObjectPool<RHI::BindingLayoutDesc, RHI::BindingLayoutPtr, true>   bindingLayoutPool_;
};

RTRC_END
