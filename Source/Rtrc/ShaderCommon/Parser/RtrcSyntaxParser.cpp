#include <ranges>

#include <Rtrc/Core/Parser/ShaderTokenStream.h>
#include <Rtrc/Core/String.h>
#include <Rtrc/ShaderCommon/Parser/ParserHelper.h>
#include <Rtrc/ShaderCommon/Parser/RtrcSyntaxParser.h>

RTRC_BEGIN

namespace BindingGroupParserDetail
{
    const std::map<std::string, RHI::BindingType, std::less<>> RESOURCE_PROPERTIES =
    {
        { "Texture1DArray",                  RHI::BindingType::Texture },
        { "Texture1D",                       RHI::BindingType::Texture },
        { "RWTexture1DArray",                RHI::BindingType::RWTexture },
        { "RWTexture1D",                     RHI::BindingType::RWTexture },
        { "Texture2DArray",                  RHI::BindingType::Texture },
        { "Texture2D",                       RHI::BindingType::Texture },
        { "RWTexture2DArray",                RHI::BindingType::RWTexture },
        { "RWTexture2D",                     RHI::BindingType::RWTexture },
        { "Texture3DArray",                  RHI::BindingType::Texture },
        { "Texture3D",                       RHI::BindingType::Texture },
        { "RWTexture3DArray",                RHI::BindingType::RWTexture },
        { "RWTexture3D",                     RHI::BindingType::RWTexture },
        { "Buffer",                          RHI::BindingType::Buffer },
        { "RWBuffer",                        RHI::BindingType::RWBuffer },
        { "StructuredBuffer",                RHI::BindingType::StructuredBuffer },
        { "RWStructuredBuffer",              RHI::BindingType::RWStructuredBuffer },
        { "ByteAddressBuffer",               RHI::BindingType::ByteAddressBuffer },
        { "RWByteAddressBuffer",             RHI::BindingType::RWByteAddressBuffer },
        { "ConstantBuffer",                  RHI::BindingType::ConstantBuffer },
        { "SamplerState",                    RHI::BindingType::Sampler },
        { "RaytracingAccelerationStructure", RHI::BindingType::AccelerationStructure }
    };

    // STAGES := STAGE (\|STAGE)*
    // STAGE := VS|FS|PS|CS|TS|MS|RT_RGS|RT_MS|RT_CHS|RT_IS|RT_AHS|Classical|Mesh|Callable|RT|RayTracing|All
    RHI::ShaderStageFlags ParseShaderStages(ShaderTokenStream &tokens)
    {
        RHI::ShaderStageFlags stages;
        while(true)
        {
            if(tokens.GetCurrentToken() == "VS")
            {
                stages |= RHI::ShaderStage::VS;
            }
            else if(tokens.GetCurrentToken() == "FS" || tokens.GetCurrentToken() == "PS")
            {
                stages |= RHI::ShaderStage::FS;
            }
            else if(tokens.GetCurrentToken() == "CS")
            {
                stages |= RHI::ShaderStage::CS;
            }
            else if(tokens.GetCurrentToken() == "TS")
            {
                stages |= RHI::ShaderStage::TS;
            }
            else if(tokens.GetCurrentToken() == "MS")
            {
                stages |= RHI::ShaderStage::MS;
            }
            else if(tokens.GetCurrentToken() == "RT_RGS")
            {
                stages |= RHI::ShaderStage::RT_RGS;
            }
            else if(tokens.GetCurrentToken() == "RT_MS")
            {
                stages |= RHI::ShaderStage::RT_MS;
            }
            else if(tokens.GetCurrentToken() == "RT_CHS")
            {
                stages |= RHI::ShaderStage::RT_CHS;
            }
            else if(tokens.GetCurrentToken() == "RT_IS")
            {
                stages |= RHI::ShaderStage::RT_IS;
            }
            else if(tokens.GetCurrentToken() == "RT_AHS")
            {
                stages |= RHI::ShaderStage::RT_AHS;
            }
            else if(tokens.GetCurrentToken() == "Classical")
            {
                stages |= RHI::ShaderStage::AllClassical;
            }
            else if(tokens.GetCurrentToken() == "Mesh")
            {
                stages |= RHI::ShaderStage::AllMesh;
            }
            else if(tokens.GetCurrentToken() == "Callable")
            {
                stages |= RHI::ShaderStage::CallableShader;
            }
            else if(tokens.GetCurrentToken() == "RT" || tokens.GetCurrentToken() == "RayTracing")
            {
                stages |= RHI::ShaderStage::AllRT;
            }
            else if(tokens.GetCurrentToken() == "All")
            {
                stages |= RHI::ShaderStage::All;
            }
            else
            {
                tokens.Throw("shader stage expected");
            }
            tokens.Next();
            if(tokens.GetCurrentToken() != "|")
            {
                break;
            }
            tokens.Next();
        }
        return stages;
    }

    ParsedBinding ParseBinding(
        ShaderTokenStream &tokens,
        bool               isBindless,
        bool               isVarSized)
    {
        ParsedBinding ret;

        std::string rawTypename = tokens.GetCurrentToken();
        auto it = RESOURCE_PROPERTIES.find(rawTypename);
        if(it == RESOURCE_PROPERTIES.end())
        {
            tokens.Throw(fmt::format("Unknown resource type: {}", tokens.GetCurrentToken()));
        }
        tokens.Next();
        const RHI::BindingType bindingType = it->second;

        std::string templateParam;
        if(bindingType != RHI::BindingType::Sampler && bindingType != RHI::BindingType::AccelerationStructure &&
           bindingType != RHI::BindingType::ByteAddressBuffer && bindingType != RHI::BindingType::RWByteAddressBuffer)
        {
            tokens.ConsumeOrThrow("<");
            while(tokens.GetCurrentToken() != ">")
            {
                if(tokens.IsFinished())
                {
                    tokens.Throw("'>' expected");
                }
                if(!templateParam.empty())
                {
                    templateParam += " ";
                }
                templateParam += tokens.GetCurrentToken();
                tokens.Next();
            }
            tokens.ConsumeOrThrow(">");
        }

        std::optional<uint32_t> arraySize;
        if(tokens.GetCurrentToken() == "[")
        {
            tokens.Next();
            try
            {
                arraySize = std::stoi(tokens.GetCurrentToken());
                tokens.Next();
            }
            catch(...)
            {
                tokens.Throw(fmt::format("Invalid array size: {}", tokens.GetCurrentToken()));
            }
            tokens.ConsumeOrThrow("]");
        }
        else if(isBindless)
        {
            tokens.Throw("Bindless item must have an array specifier");
        }

        tokens.ConsumeOrThrow(",");

        std::string bindingName = tokens.GetCurrentToken();
        if(!ShaderTokenStream::IsIdentifier(bindingName))
        {
            tokens.Throw("Binding name expected");
        }
        tokens.Next();

        RHI::ShaderStageFlags stages = RHI::ShaderStage::All;
        if(tokens.GetCurrentToken() == ",")
        {
            tokens.Next();
            stages = ParseShaderStages(tokens);
        }

        return ParsedBinding
        {
            .name              = std::move(bindingName),
            .type              = bindingType,
            .rawTypeName       = std::move(rawTypename),
            .arraySize         = arraySize,
            .stages            = stages,
            .templateParam     = std::move(templateParam),
            .bindless          = isBindless,
            .variableArraySize = isVarSized
        };
    }

    void ParseInlineSampler(ShaderTokenStream &tokens, std::string &name, RHI::SamplerDesc &desc)
    {
        desc = {};

        if(!ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
        {
            tokens.Throw("Sampler name expected");
        }
        name = tokens.GetCurrentToken();
        tokens.Next();

        while(tokens.GetCurrentToken() == ",")
        {
            tokens.Next();
            const std::string propertyName = tokens.GetCurrentToken();
            tokens.Next();
            tokens.ConsumeOrThrow("=");
            const std::string propertyValue = ToLower(tokens.GetCurrentToken());
            tokens.Next();

            auto TranslateFilterMode = [&](std::string_view str)
            {
                if(str == "nearest") { return RHI::FilterMode::Point; }
                if(str == "point") { return RHI::FilterMode::Point; }
                if(str == "linear") { return RHI::FilterMode::Linear; }
                tokens.Throw(fmt::format("Unknown filter mode: {}", str));
            };

            auto TranslateAddressMode = [&](std::string_view str)
            {
                if(str == "repeat") { return RHI::AddressMode::Repeat; }
                if(str == "mirror") { return RHI::AddressMode::Mirror; }
                if(str == "clamp") { return RHI::AddressMode::Clamp; }
                if(str == "border") { return RHI::AddressMode::Border; }
                tokens.Throw(fmt::format("Unknown address mode: ", str));
            };

            if(propertyName == "filter_mag")
            {
                desc.magFilter = TranslateFilterMode(propertyValue);
            }
            else if(propertyName == "filter_min")
            {
                desc.minFilter = TranslateFilterMode(propertyValue);
            }
            else if(propertyName == "filter_mip")
            {
                desc.mipFilter = TranslateFilterMode(propertyValue);
            }
            else if(propertyName == "filter")
            {
                desc.magFilter = desc.minFilter = desc.mipFilter = TranslateFilterMode(propertyValue);
            }
            else if(propertyName == "anisotropy")
            {
                desc.enableAnisotropy = true;
                desc.maxAnisotropy = std::stoi(propertyValue);
            }
            else if(propertyName == "address_u")
            {
                desc.addressModeU = TranslateAddressMode(propertyValue);
            }
            else if(propertyName == "address_v")
            {
                desc.addressModeV = TranslateAddressMode(propertyValue);
            }
            else if(propertyName == "address_w")
            {
                desc.addressModeW = TranslateAddressMode(propertyValue);
            }
            else if(propertyName == "address")
            {
                desc.addressModeU = desc.addressModeV = desc.addressModeW = TranslateAddressMode(propertyValue);
            }
            else
            {
                tokens.Throw(fmt::format("Unknown sampler property name: {}", propertyName));
            }
        }
    }

    void ParsePushConstantRange(
        ShaderTokenStream       &tokens,
        ParsedPushConstantRange &output,
        uint32_t                &offset)
    {
        tokens.ConsumeOrThrow("(");
        output.name = tokens.GetCurrentToken();
        tokens.Next();
        tokens.ConsumeOrThrow(",");
        if(tokens.GetCurrentToken() != ")")
        {
            output.range.stages = ParseShaderStages(tokens);
        }
        else
        {
            output.range.stages = RHI::ShaderStage::All;
        }
        tokens.ConsumeOrThrow(")");
        tokens.ConsumeOrThrow("{");

        while(true)
        {
            if(tokens.IsFinished())
            {
                tokens.Throw("'}' expected for rtrc_push_constant");
            }

            if(tokens.GetCurrentToken() == "}")
            {
                break;
            }

            const std::string typeStr = tokens.GetCurrentToken();
            tokens.Next();

            // typename -> { alignment, size, type }
            static const std::map<std::string, std::tuple<uint32_t, uint32_t, ParsedPushConstantVariable::Type>> typeInfo =
            {
                { "float",  { 4,  4,  ParsedPushConstantVariable::Float } },
                { "int",    { 4,  4,  ParsedPushConstantVariable::Int } },
                { "uint",   { 4,  4,  ParsedPushConstantVariable::UInt } },
                { "float2", { 8,  8,  ParsedPushConstantVariable::Float2 } },
                { "int2",   { 8,  8,  ParsedPushConstantVariable::Int2 } },
                { "uint2",  { 8,  8,  ParsedPushConstantVariable::UInt2 } },
                { "float3", { 16, 12, ParsedPushConstantVariable::Float3 } },
                { "int3",   { 16, 12, ParsedPushConstantVariable::Int3 } },
                { "uint3",  { 16, 12, ParsedPushConstantVariable::UInt3 } },
                { "float4", { 16, 16, ParsedPushConstantVariable::Float4 } },
                { "int4",   { 16, 16, ParsedPushConstantVariable::Int4 } },
                { "uint4",  { 16, 16, ParsedPushConstantVariable::UInt4 } },
            };

            uint32_t alignment, size; ParsedPushConstantVariable::Type type;
            if(auto it = typeInfo.find(typeStr); it != typeInfo.end())
            {
                std::tie(alignment, size, type) = it->second;
            }
            else
            {
                tokens.Throw(fmt::format("Unsupported var type {} in push constant tange {}", typeStr, output.name));
            }

            const std::string varName = tokens.GetCurrentToken();
            if(!ShaderTokenStream::IsIdentifier(varName))
            {
                tokens.Throw("Push constant variable name expected");
            }
            tokens.Next();

            tokens.ConsumeOrThrow(";");

            offset = UpAlignTo(offset, alignment);
            if(output.variables.empty())
            {
                output.range.offset = offset;
            }
            output.variables.push_back({ offset, type, varName });
            offset += size;
        }

        if(output.variables.empty())
        {
            output.range.offset = offset;
            output.range.size = 0;
        }
        else
        {
            output.range.size = offset - output.range.offset;
        }
    }

} // namespace BindingGroupParserDetail

size_t ParsedStructMember::TypeToSize(Type type)
{
    const size_t sizes[] =
    {
        4, 8, 12, 16,
        4, 8, 12, 16,
        4, 8, 12, 16
    };
    return sizes[std::to_underlying(type)];
}

const char *ParsedPushConstantVariable::TypeToName(Type type)
{
    const char *names[] =
    {
        "float",
        "float2",
        "float3",
        "float4",
        "int",
        "int2",
        "int3",
        "int4",
        "uint",
        "uint2",
        "uint3",
        "uint4"
    };
    return names[std::to_underlying(type)];
}

size_t ParsedPushConstantVariable::TypeToSize(Type type)
{
    const size_t sizes[] =
    {
        4, 8, 12, 16,
        4, 8, 12, 16,
        4, 8, 12, 16
    };
    return sizes[std::to_underlying(type)];
}

void ParseBindings(
    const std::string          &source,
    const std::string          &sourceWithoutString,
    std::vector<ParsedBinding> &bindings)
{
    bindings.clear();

    size_t keywordBeginPos = 0;
    while(true)
    {
        static const std::vector<std::string_view> KEYWORDS =
        {
            "rtrc_define",
            "rtrc_bindless",
            "rtrc_bindless_varsize"
        };
        std::string_view keyword;
        const size_t pos = FindFirstKeyword(sourceWithoutString, KEYWORDS, keywordBeginPos, keyword);
        if(pos == std::string::npos)
        {
            break;
        }
        keywordBeginPos = pos + keyword.size();

        const bool isBindless = keyword != "rtrc_define";
        const bool isVarSized = keyword == "rtrc_bindless_varsize";
        ShaderTokenStream tokens(source, keywordBeginPos);
        tokens.ConsumeOrThrow("(");
        const ParsedBinding binding = BindingGroupParserDetail::ParseBinding(tokens, isBindless, isVarSized);
        tokens.ConsumeOrThrow(")");
        bindings.push_back(binding);
    }
}

void ParseBindingGroups(
    const std::string                &source,
    const std::string                &sourceWithoutString,
    const std::vector<ParsedBinding> &allBindings,
    std::vector<ParsedBinding>       &ungroupedBindings, 
    std::vector<ParsedBindingGroup>  &groups)
{
    ungroupedBindings.clear();
    groups.clear();

    std::map<std::string, ParsedBinding> allBindingMap;
    for(auto &b : allBindings)
    {
        if(allBindingMap.contains(b.name))
        {
            throw Exception(fmt::format("Binding {} is defined twice", b.name));
        }
        allBindingMap.insert({ b.name, b });
    }
    auto ungroupedBindingMap = allBindingMap;

    std::map<std::string, ParsedBindingGroup> nameToStruct;
    std::set<std::string> parsedGroupNames;

    size_t keywordBeginPos = 0;
    while(true)
    {
        constexpr std::string_view KEYWORDS[] = {"rtrc_group", "rtrc_group_struct"};
        std::string_view keyword;
        const size_t groupPos = FindFirstKeyword(sourceWithoutString, KEYWORDS, keywordBeginPos, keyword);
        if(groupPos == std::string::npos)
        {
            break;
        }
        keywordBeginPos = groupPos + keyword.size();

        const bool isGroupStruct = keyword == "rtrc_group_struct";
        ParsedBindingGroup group;

        ShaderTokenStream tokens(source, keywordBeginPos);
        tokens.ConsumeOrThrow("(");

        group.name = tokens.GetCurrentToken();
        if(!ShaderTokenStream::IsIdentifier(group.name))
        {
            tokens.Throw("Group name expected");
        }
        if(parsedGroupNames.contains(group.name))
        {
            tokens.Throw(fmt::format("Group name {} is found twice", group.name));
        }
        parsedGroupNames.insert(group.name);
        tokens.Next();

        RHI::ShaderStageFlags groupStages = RHI::ShaderStage::All;
        if(tokens.GetCurrentToken() == ",")
        {
            tokens.Next();
            groupStages = BindingGroupParserDetail::ParseShaderStages(tokens);
        }
        group.stages = groupStages;
        tokens.ConsumeOrThrow(")");
        tokens.ConsumeOrThrow("{");

        while(true)
        {
            if(tokens.GetCurrentToken() == "rtrc_define" ||
               tokens.GetCurrentToken() == "rtrc_bindless" ||
               tokens.GetCurrentToken() == "rtrc_bindless_varsize")
            {
                const bool isBindless = tokens.GetCurrentToken() != "rtrc_define";
                const bool isVarSized = tokens.GetCurrentToken() == "rtrc_bindless_varsize";
                tokens.Next();
                tokens.ConsumeOrThrow("(");
                ParsedBinding binding = BindingGroupParserDetail::ParseBinding(tokens, isBindless, isVarSized);
                if(auto it = allBindingMap.find(binding.name); it == allBindingMap.end() || it->second != binding)
                {
                    throw Exception(fmt::format(
                        "Binding {} in group {} doesn't match its previous definition",
                        binding.name, group.name));
                }
                binding.stages = binding.stages & groupStages;
                if(binding.stages == RHI::ShaderStage::None)
                {
                    throw Exception(fmt::format(
                        "Shader stages of binding {} are incompatible with stages of group {}",
                        binding.name, group.name));
                }
                tokens.ConsumeOrThrow(")");

                if(auto it = ungroupedBindingMap.find(binding.name); it == ungroupedBindingMap.end())
                {
                    throw Exception(fmt::format("Binding {} exists in multiple groups", binding.name));
                }
                else
                {
                    ungroupedBindingMap.erase(it);
                }

                binding.definedGroupName = group.name;

                group.bindings.push_back(binding);
                group.isRef.push_back(false);
                continue;
            }

            if(tokens.GetCurrentToken() == "rtrc_ref")
            {
                tokens.Next();
                tokens.ConsumeOrThrow("(");
                const std::string refName = tokens.GetCurrentToken();
                if(!allBindingMap.contains(refName))
                {
                    throw Exception(fmt::format("Unknown binding name {} in rtrc_ref", refName));
                }
                tokens.Next();

                RHI::ShaderStageFlags refStages = RHI::ShaderStage::All;
                if(tokens.GetCurrentToken() == ",")
                {
                    tokens.Next();
                    refStages = BindingGroupParserDetail::ParseShaderStages(tokens);
                }

                tokens.ConsumeOrThrow(")");

                ParsedBinding binding;
                if(auto it = ungroupedBindingMap.find(refName); it == ungroupedBindingMap.end())
                {
                    throw Exception(fmt::format("Binding {} exists in multiple groups", refName));
                }
                else
                {
                    binding = it->second;
                    ungroupedBindingMap.erase(it);
                }

                binding.stages = binding.stages & groupStages & refStages;
                if(binding.stages == RHI::ShaderStage::None)
                {
                    throw Exception(fmt::format(
                        "Shader stages of binding {} are incompatible with stages of group {}",
                        binding.name, group.name));
                }

                group.bindings.push_back(binding);
                group.isRef.push_back(true);
                continue;
            }

            if(tokens.GetCurrentToken() == "rtrc_uniform")
            {
                tokens.Next();
                tokens.ConsumeOrThrow("(");
                ParsedUniformDefinition uniform;
                uniform.type = tokens.GetCurrentToken();
                tokens.Next();
                while(tokens.GetCurrentToken() == "::")
                {
                    tokens.Next();
                    uniform.type += "::" + tokens.GetCurrentToken();
                    tokens.Next();
                }
                tokens.ConsumeOrThrow(",");
                uniform.name = tokens.GetCurrentToken();
                if(!ShaderTokenStream::IsIdentifier(uniform.name))
                {
                    tokens.Throw("Uniform name expected");
                }
                tokens.Next();
                tokens.ConsumeOrThrow(")");

                group.uniformPropertyDefinitions.push_back(uniform);
                continue;
            }

            if(tokens.GetCurrentToken() == "rtrc_inline")
            {
                tokens.Next();
                tokens.ConsumeOrThrow("(");

                const ParsedBindingGroup *groupStruct;
                const std::string structName = tokens.GetCurrentToken();
                tokens.Next();
                if(auto it = nameToStruct.find(structName); it == nameToStruct.end())
                {
                    tokens.Throw(fmt::format("Unknown inlined binding group struct {}", structName));
                }
                else
                {
                    groupStruct = &it->second;
                }

                RHI::ShaderStageFlags inlineStages = RHI::ShaderStage::All;
                if(tokens.GetCurrentToken() == ",")
                {
                    tokens.Next();
                    inlineStages = BindingGroupParserDetail::ParseShaderStages(tokens);
                }
                tokens.ConsumeOrThrow(")");

                for(size_t i = 0; i < groupStruct->bindings.size(); ++i)
                {
                    const ParsedBinding &srcBinding = groupStruct->bindings[i];
                    const bool isRef = groupStruct->isRef[i];

                    const RHI::ShaderStageFlags stages = srcBinding.stages & groupStages & inlineStages;
                    if(stages == RHI::ShaderStage::None)
                    {
                        tokens.Throw(fmt::format(
                            "Binding {} in group struct {} (inlined in {}) doesn't have a compatible stage flag",
                            srcBinding.name, groupStruct->name, group.name));
                    }

                    group.bindings.push_back(srcBinding);
                    group.bindings.back().stages = stages;
                    group.isRef.push_back(isRef);
                }

                for(auto &uniform : groupStruct->uniformPropertyDefinitions)
                {
                    group.uniformPropertyDefinitions.push_back(uniform);
                }

                continue;
            }

            if(tokens.GetCurrentToken() == "}")
            {
                break;
            }

            tokens.Throw("rtrc_* expected (* = define/bindless/bindless_varsize/ref/uniform/inline)");
        }

        tokens.ConsumeOrThrow("}");
        tokens.ConsumeOrThrow(";");

        if(isGroupStruct)
        {
            nameToStruct.insert({ group.name, group });
        }
        else
        {
            groups.push_back(group);
        }
    }

    for(auto &binding: std::ranges::views::values(ungroupedBindingMap))
    {
        ungroupedBindings.push_back(binding);
    }
}

void ParseBindingAliases(
    const std::string               &source,
    const std::string               &sourceWithoutString,
    std::vector<ParsedBindingAlias> &aliases)
{
    aliases.clear();

    size_t keywordBeginPos = 0;
    while(true)
    {
        const size_t pos = FindKeyword(sourceWithoutString, "rtrc_alias", keywordBeginPos);
        if(pos == std::string::npos)
        {
            break;
        }
        keywordBeginPos = pos + std::string_view("rtrc_alias").size();

        ParsedBindingAlias &alias = aliases.emplace_back();

        ShaderTokenStream tokens(source, keywordBeginPos);
        tokens.ConsumeOrThrow("(");

        alias.rawTypename = tokens.GetCurrentToken();
        tokens.Next();

        auto it = BindingGroupParserDetail::RESOURCE_PROPERTIES.find(alias.rawTypename);
        if(it == BindingGroupParserDetail::RESOURCE_PROPERTIES.end())
        {
            tokens.Throw(fmt::format("Unknown resource type {}", alias.rawTypename));
        }
        alias.bindingType = it->second;

        if(alias.bindingType != RHI::BindingType::Sampler && alias.bindingType != RHI::BindingType::AccelerationStructure)
        {
            tokens.ConsumeOrThrow("<");
            while(tokens.GetCurrentToken() != ">")
            {
                if(tokens.IsFinished())
                {
                    tokens.Throw("'>' expected");
                }
                alias.templateParam += tokens.GetCurrentToken();
                tokens.Next();
            }
            tokens.ConsumeOrThrow(">");
        }

        tokens.ConsumeOrThrow(",");
        alias.name = tokens.GetCurrentToken();
        if(!ShaderTokenStream::IsIdentifier(alias.name))
        {
            tokens.Throw("Alias name expected");
        }
        tokens.Next();

        tokens.ConsumeOrThrow(",");
        alias.aliasedName = tokens.GetCurrentToken();
        if(!ShaderTokenStream::IsIdentifier(alias.aliasedName))
        {
            tokens.Throw("Aliased binding name expected");
        }
        tokens.Next();

        tokens.ConsumeOrThrow(")");
    }
}

void ParseInlineSamplers(
    const std::string             &source,
    const std::string             &sourceWithoutString,
    std::vector<RHI::SamplerDesc> &descs,
    std::map<std::string, int>    &nameToDescIndex)
{
    std::map<RHI::SamplerDesc, int> descToIndex;

    descs.clear();
    nameToDescIndex.clear();

    size_t keywordBeginPos = 0;
    while(true)
    {
        const size_t pos = FindKeyword(sourceWithoutString, "rtrc_sampler", keywordBeginPos);
        if(pos == std::string::npos)
        {
            break;
        }
        keywordBeginPos = pos + std::string_view("rtrc_sampler").size();

        ShaderTokenStream tokens(source, keywordBeginPos);
        std::string name; RHI::SamplerDesc desc;
        tokens.ConsumeOrThrow("(");
        BindingGroupParserDetail::ParseInlineSampler(tokens, name, desc);
        tokens.ConsumeOrThrow(")");
        if(auto it = nameToDescIndex.find(name); it != nameToDescIndex.end())
        {
            throw Exception(fmt::format("Sampler {} is defined twice", name));
        }
        if(auto it = descToIndex.find(desc); it != descToIndex.end())
        {
            nameToDescIndex.insert({ name, it->second });
            continue;
        }
        const int newIndex = descs.size();
        descs.push_back(desc);
        nameToDescIndex.insert({ name, newIndex });
        descToIndex.insert({ desc, newIndex });
    }
}

void ParsePushConstantRanges(
    const std::string                    &source,
    const std::string                    &sourceWithoutString,
    std::vector<ParsedPushConstantRange> &ranges)
{
    ranges.clear();
    uint32_t nextPushConstantOffset = 0;

    size_t keywordBeginPos = 0;
    while(true)
    {
        const size_t pos = FindKeyword(sourceWithoutString, "rtrc_push_constant", keywordBeginPos);
        if(pos == std::string::npos)
        {
            break;
        }
        keywordBeginPos = pos + std::string_view("rtrc_push_constant").size();

        ShaderTokenStream tokens(source, keywordBeginPos);
        BindingGroupParserDetail::ParsePushConstantRange(
            tokens, ranges.emplace_back(), nextPushConstantOffset);
    }
}

void ParseSpecialStructs(
    const std::string                   &source,
    std::vector<ParsedStructDefinition> &structs)
{
    size_t keywordBeginPos = 0;
    while(true)
    {
        constexpr std::string_view keyword = "rtrc_refl_struct";
        const size_t keywordPos = FindKeyword(source, keyword, keywordBeginPos);
        if(keywordPos == std::string::npos)
        {
            break;
        }
        keywordBeginPos = keywordPos + keyword.size();

        ShaderTokenStream tokens(source, keywordBeginPos);
        tokens.ConsumeOrThrow("(");
        std::string name = tokens.GetCurrentToken();
        if(!ShaderTokenStream::IsIdentifier(name))
        {
            tokens.Throw("rtrc_refl_struct name expected");
        }
        tokens.Next();
        tokens.ConsumeOrThrow(")");
        tokens.ConsumeOrThrow("{");

        uint32_t offset = 0;
        std::vector<ParsedStructMember> members;
        while(true)
        {
            if(tokens.IsFinished())
            {
                tokens.Throw("'}' expected for rtrc_push_constant");
            }

            if(tokens.GetCurrentToken() == "}")
            {
                break;
            }

            const std::string typeStr = tokens.GetCurrentToken();
            tokens.Next();

            // typename -> { alignment, size }
            static const std::map<std::string, std::tuple<uint32_t, uint32_t, ParsedStructMember::Type>> typeInfo =
            {
                { "float",  { 4,  4,  ParsedStructMember::Float } },
                { "int",    { 4,  4,  ParsedStructMember::Int } },
                { "uint",   { 4,  4,  ParsedStructMember::UInt } },
                { "float2", { 8,  8,  ParsedStructMember::Float2 } },
                { "int2",   { 8,  8,  ParsedStructMember::Int2 } },
                { "uint2",  { 8,  8,  ParsedStructMember::UInt2 } },
                { "float3", { 16, 12, ParsedStructMember::Float3 } },
                { "int3",   { 16, 12, ParsedStructMember::Int3 } },
                { "uint3",  { 16, 12, ParsedStructMember::UInt3 } },
                { "float4", { 16, 16, ParsedStructMember::Float4 } },
                { "int4",   { 16, 16, ParsedStructMember::Int4 } },
                { "uint4",  { 16, 16, ParsedStructMember::UInt4 } },
            };

            uint32_t alignment, size; ParsedStructMember::Type type;
            if(auto it = typeInfo.find(typeStr); it != typeInfo.end())
            {
                std::tie(alignment, size, type) = it->second;
            }
            else
            {
                tokens.Throw(fmt::format("Unsupported var type {} in rtrc_refl_struct {}", typeStr, name));
            }

            std::string varName = tokens.GetCurrentToken();
            if(!ShaderTokenStream::IsIdentifier(varName))
            {
                tokens.Throw("Push constant variable name expected");
            }
            tokens.Next();

            tokens.ConsumeOrThrow(";");

            offset = UpAlignTo(offset, alignment);
            members.push_back({ type, std::move(varName) });
            offset += size;
        }
        tokens.ConsumeOrThrow("}");
        tokens.ConsumeOrThrow(";");

        auto &s = structs.emplace_back();
        s.name = std::move(name);
        s.members = std::move(members);
        s.size = offset;
    }
}

RTRC_END
