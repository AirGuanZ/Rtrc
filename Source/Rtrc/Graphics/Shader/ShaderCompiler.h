#pragma once

#include <filesystem>

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Shader/Shader.h>

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

    void SetDevice(Device *device);
    void SetRootDirectory(std::string_view rootDir);

    RC<Shader> Compile(
        const ShaderSource &desc,
        const Macros       &macros = {},
        bool                debug = RTRC_DEBUG);

private:

    std::string MapFilename(std::string_view filename);

    RHI::RawShaderPtr CompileStage(
        RHI::ShaderStage                         stage,
        const std::string                       &source,
        const std::string                       &filename,
        const std::string                       &entry,
        bool                                     debug,
        const Macros                            &macros,
        std::map<std::string, int, std::less<>> &nameToGroupIndex,
        std::vector<RC<BindingGroupLayout>>     &groupLayouts,
        std::vector<std::string>                &groupNames,
        Box<ShaderReflection>                   &refl);

    Device *device_ = nullptr;
    std::filesystem::path rootDir_;
};

RTRC_END
