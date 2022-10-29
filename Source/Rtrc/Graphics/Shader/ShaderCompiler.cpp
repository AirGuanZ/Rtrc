#include <ranges>

#include <Rtrc/Graphics/Shader/DXC.h>
#include <Rtrc/Graphics/Shader/ShaderCompiler.h>
#include <Rtrc/Graphics/Shader/ShaderTokenStream.h>
#include <Rtrc/Utils/Enumerate.h>
#include <Rtrc/Utils/File.h>

RTRC_BEGIN

namespace
{

    struct ParsedBinding
    {
        std::string             name;
        RHI::BindingType        type = {};
        std::optional<uint32_t> arraySize;
        RHI::ShaderStageFlag    stages = {};
        std::string             templateParam;
    };

    struct ParsedBindingGroup
    {
        std::string name;
        std::string valuePropertyDefinitions;
        std::vector<ParsedBinding> bindings;
    };

    size_t FindKeyword(const std::string &source, const std::string &keyword, size_t beginPos)
    {
        while(true)
        {
            size_t p = source.find(keyword, beginPos);
            if(p == std::string::npos)
            {
                return std::string::npos;
            }
            const bool isPrevCharOk = p == 0 || ShaderTokenStream::IsNonIdentifierChar(source[p - 1]);
            const bool isNextCharOk = ShaderTokenStream::IsNonIdentifierChar(source[p + keyword.size()]);
            if(isPrevCharOk && isNextCharOk)
            {
                return p;
            }
            beginPos = p + keyword.size();
        }
    }

    const std::set<std::string, std::less<>> VALUE_PROPERTIES =
    {
        "float",
        "float2",
        "float3",
        "float4",
        "uint",
        "uint2",
        "uint3",
        "uint4",
        "int",
        "int2",
        "int3",
        "int4"
    };

    const std::map<std::string, RHI::BindingType, std::less<>> RESOURCE_PROPERTIES =
    {
        { "Texture2DArray",     RHI::BindingType::Texture2DArray },
        { "Texture2D",          RHI::BindingType::Texture2D },
        { "RWTexture2DArray",   RHI::BindingType::RWTexture2DArray },
        { "RWTexture2D",        RHI::BindingType::RWTexture2D },
        { "Texture3DArray",     RHI::BindingType::Texture3DArray },
        { "Texture3D",          RHI::BindingType::Texture3D },
        { "RWTexture3DArray",   RHI::BindingType::RWTexture3DArray },
        { "RWTexture3D",        RHI::BindingType::RWTexture3D },
        { "Buffer",             RHI::BindingType::Buffer },
        { "RWBuffer",           RHI::BindingType::RWBuffer },
        { "StructuredBuffer",   RHI::BindingType::StructuredBuffer },
        { "RWStructuredBuffer", RHI::BindingType::RWStructuredBuffer },
        { "ConstantBuffer",     RHI::BindingType::ConstantBuffer },
        { "SamplerState",       RHI::BindingType::Sampler }
    };

    const char *BindingTypeToTypeName(RHI::BindingType type)
    {
        static const char *NAMES[] =
        {
            "Texture2D",
            "RWTexture2D",
            "Texture3D",
            "RWTexture3D",
            "Texture2DArray",
            "RWTexture2DArray",
            "Texture3DArray",
            "RWTexture3DArray",
            "Buffer",
            "StructuredBuffer",
            "RWBuffer",
            "RWStructuredBuffer",
            "ConstantBuffer",
            "SamplerState",
        };
        assert(static_cast<int>(type) < GetArraySize<int>(NAMES));
        return NAMES[static_cast<int>(type)];
    }

    RHI::ShaderStageFlag ParseStages(ShaderTokenStream &tokens)
    {
        RHI::ShaderStageFlag stages;
        while(true)
        {
            if(tokens.GetCurrentToken() == "VS")
            {
                stages |= RHI::ShaderStageFlags::VS;
            }
            else if(tokens.GetCurrentToken() == "FS" || tokens.GetCurrentToken() == "PS")
            {
                stages |= RHI::ShaderStageFlags::FS;
            }
            else if(tokens.GetCurrentToken() == "CS")
            {
                stages |= RHI::ShaderStageFlags::CS;
            }
            else if(tokens.GetCurrentToken() == "All")
            {
                stages |= RHI::ShaderStageFlags::All;
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

    template<bool AllowStageSpecifier>
    ParsedBinding ParseBinding(ShaderTokenStream &tokens)
    {
        ParsedBinding ret;
        
        auto it = RESOURCE_PROPERTIES.find(tokens.GetCurrentToken());
        if(it == RESOURCE_PROPERTIES.end())
        {
            tokens.Throw(fmt::format("unknown resource type: {}", tokens.GetCurrentToken()));
        }
        tokens.Next();
        const RHI::BindingType bindingType = it->second;

        std::string templateParam;
        if(bindingType != RHI::BindingType::Sampler)
        {
            tokens.ConsumeOrThrow("<");
            templateParam = tokens.GetCurrentToken();
            if(!ShaderTokenStream::IsIdentifier(templateParam))
            {
                tokens.Throw("resource template parameter expected");
            }
            tokens.Next();
            tokens.ConsumeOrThrow(">");
        }

        std::optional<uint32_t> arraySize;
        if(tokens.GetCurrentToken() == "[")
        {
            tokens.Next();
            try
            {
                arraySize = std::stoi(tokens.GetCurrentToken());
            }
            catch(...)
            {
                tokens.Throw(fmt::format("invalid array size: {}", tokens.GetCurrentToken()));
            }
            tokens.ConsumeOrThrow("]");
        }

        tokens.ConsumeOrThrow(",");

        std::string bindingName = tokens.GetCurrentToken();
        if(!ShaderTokenStream::IsIdentifier(bindingName))
        {
            tokens.Throw("binding name expected");
        }
        tokens.Next();

        RHI::ShaderStageFlag stages = RHI::ShaderStageFlags::All;
        if(tokens.GetCurrentToken() == ",")
        {
            tokens.Next();
            stages = ParseStages(tokens);
        }

        return ParsedBinding
        {
            .name          = std::move(bindingName),
            .type          = bindingType,
            .arraySize     = arraySize,
            .stages        = stages,
            .templateParam = std::move(templateParam)
        };
    }

    std::vector<ParsedBindingGroup> ParseBindingGroups(std::string &source)
    {
        std::vector<ParsedBindingGroup> groups;
        std::set<std::string> parsedGroupNames;
        std::map<std::string, ParsedBinding> ungroupedBindings;

        // unparsed bindingName -> (index in 'groups', index in group)
        std::map<std::string, std::pair<int, int>> bindingNameToGroupLocation;
        std::set<std::string> parsedBindingNames;

        const std::string GROUP_BEGIN_KEYWORD  = "rtrc_group";
        const std::string GROUP_DEFINE_KEYWORD = "rtrc_define";
        const std::string GROUP_END_KEYWORD    = "rtrc_end";
        const std::string RESOURCE_KEYWORD     = "rtrc_define";

        size_t keywordBeginPos = 0;
        while(true)
        {
            const size_t resourcePos = FindKeyword(source, RESOURCE_KEYWORD, keywordBeginPos);
            const size_t groupPos = FindKeyword(source, GROUP_BEGIN_KEYWORD, keywordBeginPos);

            if(resourcePos < groupPos)
            {
                keywordBeginPos = resourcePos + RESOURCE_KEYWORD.size();
                ShaderTokenStream tokens(source, resourcePos + RESOURCE_KEYWORD.size());
                tokens.ConsumeOrThrow("(");
                auto binding = ParseBinding<false>(tokens);
                if(parsedBindingNames.contains(binding.name))
                {
                    tokens.Throw(fmt::format("binding {} is defined multiple times", binding.name));
                }
                parsedBindingNames.insert(binding.name);
                ungroupedBindings.insert({ binding.name, binding });
                tokens.ConsumeOrThrow(")");
                continue;
            }

            if(groupPos == std::string::npos)
            {
                break;
            }
            
            ShaderTokenStream tokens(source, groupPos + GROUP_BEGIN_KEYWORD.size());

            tokens.ConsumeOrThrow("(");
            std::string groupName = tokens.GetCurrentToken();
            if(!ShaderTokenStream::IsIdentifier(groupName))
            {
                tokens.Throw("Group name expected");
            }
            tokens.Next();
            tokens.ConsumeOrThrow(")");

            if(parsedGroupNames.contains(groupName))
            {
                tokens.Throw(fmt::format("group {} is already defined", groupName));
            }
            parsedGroupNames.insert(groupName);

            ParsedBindingGroup &currentGroup = groups.emplace_back();
            currentGroup.name = std::move(groupName);

            while(true)
            {
                if(tokens.GetCurrentToken() == "rtrc_define")
                {
                    tokens.Next();
                    tokens.ConsumeOrThrow("(");

                    if(RESOURCE_PROPERTIES.contains(tokens.GetCurrentToken()))
                    {
                        currentGroup.bindings.push_back(ParseBinding<true>(tokens));
                        const std::string &bindingName = currentGroup.bindings.back().name;
                        if(parsedBindingNames.contains(bindingName))
                        {
                            tokens.Throw(fmt::format("resource {} is defined for multiple times", bindingName));
                        }
                        parsedBindingNames.insert(bindingName);
                    }
                    else
                    {
                        if(!ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
                        {
                            tokens.Throw(fmt::format("invalid binding name: {}", tokens.GetCurrentToken()));
                        }

                        auto it = ungroupedBindings.find(tokens.GetCurrentToken());
                        if(it == ungroupedBindings.end())
                        {
                            tokens.Throw(fmt::format(
                                "{} wasn't declared as a resource binding", tokens.GetCurrentToken()));
                        }

                        currentGroup.bindings.push_back(it->second);
                        ungroupedBindings.erase(it);
                        tokens.Next();

                        if(tokens.GetCurrentToken() == ",")
                        {
                            tokens.Next();
                            currentGroup.bindings.back().stages = ParseStages(tokens);
                        }
                        else
                        {
                            currentGroup.bindings.back().stages = RHI::ShaderStageFlags::All;
                        }
                    }

                    tokens.ConsumeOrThrow(")");
                }
                else if(tokens.GetCurrentToken() == "rtrc_uniform")
                {
                    tokens.Next();
                    tokens.ConsumeOrThrow("(");
                    if(!VALUE_PROPERTIES.contains(tokens.GetCurrentToken()))
                    {
                        tokens.Throw(fmt::format("unknown value property type: {}", tokens.GetCurrentToken()));
                    }
                    currentGroup.valuePropertyDefinitions += tokens.GetCurrentToken();
                    tokens.Next();
                    tokens.ConsumeOrThrow(",");
                    if(!ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
                    {
                        tokens.Throw("uniform property name expected");
                    }
                    currentGroup.valuePropertyDefinitions += fmt::format(" {};", tokens.GetCurrentToken());
                    tokens.Next();
                    tokens.ConsumeOrThrow(")");
                }
                else if(tokens.GetCurrentToken() == "rtrc_end")
                {
                    keywordBeginPos = tokens.GetCurrentPosition();
                    break;
                }
                else
                {
                    tokens.Throw("'rtrc_define' or 'rtrc_end' expected");
                }
            }
        }

        if(!ungroupedBindings.empty())
        {
            throw Exception(fmt::format(
                "Binding ({}, ...) {} not grouped",
                ungroupedBindings.begin()->first,
                ungroupedBindings.size() >= 2 ? "are" : "is"));
        }

        return groups;
    }

} // namespace anonymous

void ShaderCompiler::SetRenderContext(RenderContext *renderContext)
{
    renderContext_ = renderContext;
}

void ShaderCompiler::SetRootDirectory(std::string_view rootDir)
{
    rootDir_ = std::filesystem::absolute(rootDir);
}

RC<Shader> ShaderCompiler::Compile(const ShaderSource &desc, const Macros &macros, bool debug)
{
    std::map<std::string, int, std::less<>> nameToGroupIndex;
    std::vector<RC<BindingGroupLayout>>     groupLayouts;
    std::vector<std::string>                groupNames;

    auto shader = MakeRC<Shader>();

    std::string source = desc.source;
    if(source.empty())
    {
        assert(!desc.filename.empty());
        source = File::ReadTextFile(MapFilename(desc.filename));
    }

    std::string filename = desc.filename;
    if(filename.empty())
    {
        filename = "anonymous.hlsl";
    }

    Box<ShaderReflection> VSRefl, FSRefl, CSRefl;
    if(!desc.VSEntry.empty())
    {
        shader->VS_ = CompileStage(
            RHI::ShaderStage::VertexShader, source, filename, desc.VSEntry,
            debug, macros, nameToGroupIndex, groupLayouts, groupNames, VSRefl);
        shader->VSInput_ = VSRefl->GetInputVariables();
    }
    if(!desc.FSEntry.empty())
    {
        shader->FS_ = CompileStage(
            RHI::ShaderStage::FragmentShader, source, filename, desc.FSEntry,
            debug, macros, nameToGroupIndex, groupLayouts, groupNames, FSRefl);
    }
    if(!desc.CSEntry.empty())
    {
        shader->CS_ = CompileStage(
            RHI::ShaderStage::ComputeShader, source, filename, desc.CSEntry,
            debug, macros, nameToGroupIndex, groupLayouts, groupNames, CSRefl);
    }

    std::map<std::pair<int, int>, ShaderConstantBuffer> slotToCBuffer;
    auto MergeCBuffers = [&](const ShaderReflection &refl)
    {
        auto cbuffers = refl.GetConstantBuffers();
        for(auto &cbuffer : cbuffers)
        {
            const std::pair key{ cbuffer.group, cbuffer.indexInGroup };
            auto it = slotToCBuffer.find(key);
            if(it == slotToCBuffer.end())
            {
                slotToCBuffer.insert({ key, cbuffer });
            }
#if RTRC_DEBUG
            else
            {
                assert(it->second == cbuffer);
            }
#endif
        }
    };
    if(VSRefl)
    {
        MergeCBuffers(*VSRefl);
    }
    if(FSRefl)
    {
        MergeCBuffers(*FSRefl);
    }
    if(CSRefl)
    {
        MergeCBuffers(*CSRefl);
    }
    std::ranges::copy(std::ranges::views::values(slotToCBuffer), std::back_inserter(shader->constantBuffers_));

    shader->nameToBindingGroupLayoutIndex_ = std::move(nameToGroupIndex);
    shader->bindingGroupLayouts_ = std::move(groupLayouts);
    shader->bindingGroupNames_ = std::move(groupNames);

    BindingLayout::Desc bindingLayoutDesc;
    for(auto &group : shader->bindingGroupLayouts_)
    {
        bindingLayoutDesc.groupLayouts.push_back(group);
    }
    shader->bindingLayout_ = renderContext_->CreateBindingLayout(bindingLayoutDesc);

    return shader;
}

std::string ShaderCompiler::MapFilename(std::string_view filename)
{
    std::filesystem::path path = filename;
    path = path.lexically_normal();
    if(path.is_relative())
    {
        path = rootDir_ / path;
    }
    return absolute(path).lexically_normal().string();
}

RHI::RawShaderPtr ShaderCompiler::CompileStage(
    RHI::ShaderStage                         stage,
    const std::string                       &source,
    const std::string                       &filename,
    const std::string                       &entry,
    bool                                     debug,
    const Macros                            &macros,
    std::map<std::string, int, std::less<>> &nameToGroupIndex,
    std::vector<RC<BindingGroupLayout>>     &groupLayouts,
    std::vector<std::string>                &groupNames,
    Box<ShaderReflection>                   &refl)
{
    DXC::Target dxcTarget = {};
    switch(stage)
    {
    case RHI::ShaderStage::VertexShader:   dxcTarget = DXC::Target::Vulkan_1_3_VS_6_0; break;
    case RHI::ShaderStage::FragmentShader: dxcTarget = DXC::Target::Vulkan_1_3_FS_6_0; break;
    case RHI::ShaderStage::ComputeShader:  dxcTarget = DXC::Target::Vulkan_1_3_CS_6_0; break;
    }

    // Preprocess

    std::vector<std::string> includeDirs = { rootDir_.string()};
    if(!filename.empty())
    {
        includeDirs.push_back(
            absolute(std::filesystem::path(filename)).parent_path().lexically_normal().string());
    }

    DXC::ShaderInfo shaderInfo =
    {
        .source         = source,
        .sourceFilename = filename,
        .entryPoint     = entry,
        .includeDirs    = includeDirs,
        .macros         = macros
    };
    DXC dxc;
    std::string preprocessed;
    dxc.Compile(shaderInfo, dxcTarget, debug, &preprocessed);

    // Parse resources & binding groups

    auto parsedBindingGroups = ParseBindingGroups(preprocessed);

    // Generate groups

    std::map<std::string, std::pair<int, int>> bindingNameToFinalSlots; // bindingName -> (set, slot)
    for(auto &group : parsedBindingGroups)
    {
        RHI::BindingGroupLayoutDesc rhiLayoutDesc;
        rhiLayoutDesc.name = group.name;

        for(auto &binding : group.bindings)
        {
            RHI::BindingDesc rhiBindingDesc;
            rhiBindingDesc.name = binding.name;
            rhiBindingDesc.type = binding.type;
            rhiBindingDesc.shaderStages = binding.stages;
            rhiBindingDesc.arraySize = binding.arraySize;
            rhiLayoutDesc.bindings.push_back({ rhiBindingDesc });
        }

        if(!group.valuePropertyDefinitions.empty())
        {
            rhiLayoutDesc.bindings.push_back({ RHI::BindingDesc
            {
                .name = group.name,
                .type = RHI::BindingType::ConstantBuffer,
                .shaderStages = RHI::ShaderStageFlags::All
            } });
        }

        if(auto it = nameToGroupIndex.find(group.name); it != nameToGroupIndex.end())
        {
            if(rhiLayoutDesc != groupLayouts[it->second]->GetRHIObject()->GetDesc())
            {
                throw Exception(fmt::format(
                    "binding group {} appears differently in different stages", group.name));
            }
            for(auto &&[slotIndex, binding] : Enumerate(group.bindings))
            {
                bindingNameToFinalSlots[binding.name] = { it->second, static_cast<int>(slotIndex) };
            }
        }
        else
        {
            auto groupLayout = renderContext_->CreateBindingGroupLayout(rhiLayoutDesc);
            const int groupIndex = static_cast<int>(groupLayouts.size());

            nameToGroupIndex.insert({ group.name, groupIndex });
            groupLayouts.push_back(groupLayout);
            groupNames.push_back(group.name);

            for(auto &&[slotIndex, binding] : Enumerate(group.bindings))
            {
                bindingNameToFinalSlots[binding.name] = { groupIndex, static_cast<int>(slotIndex) };
            }
        }
    }

    // Generate macros

    Macros actualMacros = macros;
    actualMacros.insert({ "rtrc_group(NAME)", "_rtrc_group_##NAME" });
    actualMacros.insert({ "rtrc_define(TYPE, NAME)", "_rtrc_resource_##NAME" });
    actualMacros.insert({ "rtrc_end", "" });
    actualMacros.insert({ "rtrc_uniform(A, B)", "" });

    for(auto &group : parsedBindingGroups)
    {
        for(auto &binding : group.bindings)
        {
            const auto [set, slot] = bindingNameToFinalSlots.at(binding.name);
            std::string macroLeft = fmt::format("_rtrc_resource_{}", binding.name);
            std::string macroRight = fmt::format(
                "[[vk::binding({}, {})]] {}{} {}{};",
                slot, set,
                BindingTypeToTypeName(binding.type),
                binding.templateParam.empty() ? std::string{} : fmt::format("<{}>", binding.templateParam),
                binding.name,
                binding.arraySize ? fmt::format("[{}]", *binding.arraySize) : std::string{});
            actualMacros.insert({ std::move(macroLeft), std::move(macroRight) });
        }

        std::string groupMacroLeft = fmt::format("_rtrc_group_{}", group.name);
        std::string groupMacroRight;
        if(!group.valuePropertyDefinitions.empty())
        {
            groupMacroRight = fmt::format(
                "struct _rtrcGeneratedStruct{} {{ {} }}; "
                "[[vk::binding({}, {})]] ConstantBuffer<_rtrcGeneratedStruct{}> {};",
                group.name, group.valuePropertyDefinitions, group.bindings.size(),
                nameToGroupIndex.at(group.name), group.name, group.name);
        }
        actualMacros.insert({ std::move(groupMacroLeft), std::move(groupMacroRight) });
    }

    shaderInfo.macros = actualMacros;
    const std::vector<unsigned char> compileData = dxc.Compile(shaderInfo, dxcTarget, debug, nullptr);
    refl = MakeBox<SPIRVReflection>(compileData, entry);
    return renderContext_->GetRawDevice()->CreateShader(compileData.data(), compileData.size(), entry, stage);
}

RTRC_END
