#pragma once

#include <filesystem>
#include <map>

#include <Rtrc/Graphics/Shader/Shader.h>
#include <Rtrc/Utils/SharedObjectPool.h>

RTRC_BEGIN

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
        const ShaderSource &desc, bool debug = RTRC_DEBUG, const Macros &macros = {}, bool skipPreprocess = false);

    std::string Preprocess(std::string source, std::string filename, const Macros &macros);

    std::string GetMappedFilename(const std::string &filename);

    RC<BindingGroupLayout> GetBindingGroupLayout(const RHI::BindingGroupLayoutDesc &desc);

private:

    RHI::RawShaderPtr CompileShader(
        const std::string                        &source,
        const std::string                        &filename,
        const std::string                        &entry,
        bool                                      debug,
        const std::map<std::string, std::string> &macros,
        RHI::ShaderStage                          stage,
        bool                                      skipPreprocess,
        std::map<std::string, int, std::less<>>  &outputNameToGroupIndex,
        std::vector<RC<BindingGroupLayout>>      &outBindingGroupLayouts,
        Box<ShaderReflection>                    &outputRefl);

    RHI::DevicePtr rhiDevice_;
    std::filesystem::path rootDir_;

    SharedObjectPool<RHI::BindingGroupLayoutDesc, BindingGroupLayout, true> bindingGroupLayoutPool_;
    SharedObjectPool<RHI::BindingLayoutDesc, Shader::BindingLayout, true> bindingLayoutPool_;
};

RTRC_END
