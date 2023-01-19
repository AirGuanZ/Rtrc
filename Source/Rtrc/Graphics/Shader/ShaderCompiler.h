#pragma once

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Shader/Shader.h>

RTRC_BEGIN

class ShaderTokenStream;

class ShaderCompiler : public Uncopyable
{
public:

    using Macros = std::map<std::string, std::string>;

    struct ShaderSource
    {
        std::string filename;
        std::string source;

        std::string vsEntry;
        std::string fsEntry;
        std::string csEntry;
    };

    void SetDevice(Device *device);
    void SetRootDirectory(std::string_view rootDir);
    void AddIncludeDirectory(std::string_view dir);

    RC<Shader> Compile(
        const ShaderSource &source,
        const Macros       &macros = {},
        bool                debug = RTRC_DEBUG) const;

private:

    struct ParsedBinding
    {
        std::string             name;
        RHI::BindingType        type = {};
        std::optional<uint32_t> arraySize;
        RHI::ShaderStageFlag    stages = {};
        std::string             templateParam;
        bool                    bindless = false;
    };

    struct ParsedBindingGroup
    {
        std::string name;
        std::string valuePropertyDefinitions;
        std::vector<ParsedBinding> bindings;
        std::vector<bool>          isRef;
    };

    struct Bindings
    {
        // User defined groups

        std::vector<ParsedBindingGroup>         groups;
        std::vector<std::string>                groupNames;
        std::vector<ParsedBinding>              ungroupedBindings;
        std::map<std::string, int, std::less<>> ungroupedBindingNameToSlot;
        std::map<std::string, int, std::less<>> nameToGroupIndex;

        // Inline samplers

        std::vector<RHI::SamplerDesc>   inlineSamplerDescs;
        std::map<std::string, int>      inlineSamplerNameToDescIndex;

        // Push constants

        std::vector<Shader::PushConstantRange> pushConstantRanges;
        std::vector<std::string>               pushConstantRangeContents;
        std::vector<std::string>               pushConstantRangeNames;
    };

    std::string MapFilename(std::string_view filename) const;

    template<bool AllowStageSpecifier>
    ParsedBinding ParseBinding(ShaderTokenStream &tokens) const;

    void ParseInlineSampler(ShaderTokenStream &tokens, std::string &name, RHI::SamplerDesc &desc) const;

    void ParsePushConstantRange(
        ShaderTokenStream &tokens, std::string &name, std::string &content,
        Shader::PushConstantRange &range, uint32_t &nextOffset) const;

    Bindings CollectBindings(const std::string &source) const;

    Device *device_ = nullptr;
    std::filesystem::path rootDir_;
    std::set<std::string> includeDirs_;
};

RTRC_END
