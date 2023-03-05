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
    };

    void SetDevice(Device *device);
    void AddIncludeDirectory(std::string_view dir);

    RC<Shader> Compile(
        const ShaderSource &source,
        const Macros       &macros = {},
        bool                debug = RTRC_DEBUG) const;

private:

    struct ParsedRayGenShaderGroup { std::string entry; };
    struct ParsedMissShaderGroup { std::string entry; };
    struct ParsedHitShaderGroup
    {
        std::string closestHitEntry;
        std::string anyHitEntry;
        std::string intersectionEntry;
    };

    struct ParsedShaderEntry
    {
        std::string vertexEntry;
        std::string fragmentEntry;
        std::string computeEntry;

        bool isRayTracingShader = false;
        std::vector<std::vector<std::string>> shaderGroups;
    };

    struct ParsedBindingAlias
    {
        std::string      name;
        std::string      aliasedName;
        //std::string      namespaceChain;
        RHI::BindingType type;
        std::string      rawTypename;
        std::string      templateParam;
    };

    struct ParsedBinding
    {
        std::string             name;
        //std::string             namespaceChain;
        RHI::BindingType        type = {};
        std::string             rawTypename;
        std::optional<uint32_t> arraySize;
        RHI::ShaderStageFlag    stages = {};
        std::string             templateParam;
        bool                    bindless = false;
        bool                    variableArraySize = false;

        // "[3]" or "[]"
        std::string GetArraySpeficier() const;
    };

    struct ParsedUniformDefinition
    {
        std::string type;
        std::string name;
    };

    struct ParsedBindingGroup
    {
        std::string                          name;
        std::vector<ParsedUniformDefinition> uniformPropertyDefinitions;
        std::vector<ParsedBinding>           bindings;
        std::vector<bool>                    isRef;
        RHI::ShaderStageFlag                 defaultStages;
    };

    struct Bindings
    {
        // User defined groups

        std::vector<ParsedBindingGroup>         groups;
        std::vector<std::string>                groupNames;
        std::vector<ParsedBinding>              ungroupedBindings;
        std::map<std::string, int, std::less<>> ungroupedBindingNameToSlot;
        std::map<std::string, int, std::less<>> nameToGroupIndex;

        // Aliases

        std::vector<ParsedBindingAlias> aliases;

        // Inline samplers

        std::vector<RHI::SamplerDesc> inlineSamplerDescs;
        std::map<std::string, int>    inlineSamplerNameToDescIndex;

        // Push constants

        std::vector<Shader::PushConstantRange> pushConstantRanges;
        std::vector<std::string>               pushConstantRangeContents;
        std::vector<std::string>               pushConstantRangeNames;
    };

    enum class BindingCategory
    {
        Regular,
        Bindless,
        BindlessWithVariableSize
    };

    ParsedShaderEntry ParseShaderEntry(std::string &source) const;

    template<bool AllowStageSpecifier, BindingCategory Category>
    ParsedBinding ParseBinding(ShaderTokenStream &tokens, RHI::ShaderStageFlag groupDefaultStages) const;

    ParsedBindingAlias ParseBindingAlias(ShaderTokenStream &tokens) const;

    void ParseInlineSampler(ShaderTokenStream &tokens, std::string &name, RHI::SamplerDesc &desc) const;

    void ParsePushConstantRange(
        ShaderTokenStream &tokens, std::string &name, std::string &content,
        Shader::PushConstantRange &range, uint32_t &nextOffset, Shader::Category category) const;

    Bindings CollectBindings(const std::string &source, Shader::Category category) const;

    static std::string GetNamespaceChain(const std::string &source, size_t endPos);
    
    Device *device_ = nullptr;
    std::filesystem::path rootDir_;
    std::set<std::string> includeDirs_;
};

RTRC_END
