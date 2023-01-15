#include <ranges>

#include <Rtrc/Graphics/Shader/DXC.h>
#include <Rtrc/Graphics/Shader/ShaderCompiler.h>
#include <Rtrc/Graphics/Shader/ShaderTokenStream.h>
#include <Rtrc/Utility/Enumerate.h>
#include <Rtrc/Utility/File.h>
#include <Rtrc/Utility/String.h>

RTRC_BEGIN

namespace ShaderCompilerDetail
{

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

    size_t FindKeyword(const std::string &source, std::string_view keyword, size_t beginPos)
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

    RHI::ShaderStageFlag ParseStages(ShaderTokenStream &tokens)
    {
        RHI::ShaderStageFlag stages;
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

} // namespace ShaderCompilerDetail

void ShaderCompiler::SetDevice(Device *device)
{
    device_ = device;
}

void ShaderCompiler::SetRootDirectory(std::string_view rootDir)
{
    rootDir_ = std::filesystem::absolute(rootDir).lexically_normal();
    includeDirs_.insert(rootDir_.string());
}

void ShaderCompiler::AddIncludeDirectory(std::string_view dir)
{
    includeDirs_.insert(std::filesystem::absolute(dir).lexically_normal().string());
}

RC<Shader> ShaderCompiler::Compile(const ShaderSource &source, const Macros &macros, bool debug) const
{
    std::string sourceFromFile;
    const std::string *pSource;
    if(source.source.empty())
    {
        assert(!source.filename.empty());
        sourceFromFile = File::ReadTextFile(source.filename);
        pSource = &sourceFromFile;
    }
    else
    {
        pSource = &source.source;
    }

    std::vector<std::string> includeDirs;
    for(auto &d : includeDirs_)
    {
        includeDirs.push_back(d);
    }
    if(!source.filename.empty())
    {
        std::string dir = absolute(std::filesystem::path(source.filename)).parent_path().lexically_normal().string();
        includeDirs.push_back(std::move(dir));
    }

    DXC dxc;
    DXC::ShaderInfo shaderInfo =
    {
        .source = *pSource,
        .sourceFilename = source.filename.empty() ? "anonymous.hlsl" : source.filename,
        .includeDirs = std::move(includeDirs),
        .macros = macros
    };
    shaderInfo.macros.insert({ "rtrc_push_constant(...)", "_rtrc_push_constant_impl2(__COUNTER__, __VA_ARGS__)" });
    shaderInfo.macros.insert({ "_rtrc_push_constant_impl2(...)", "_rtrc_push_constant_impl(__VA_ARGS__)" });
    
    for(size_t i = 0; i < shaderInfo.source.size() && shaderInfo.source[i] != '\n'; ++i)
    {
        if(!std::isspace(shaderInfo.source[i]))
        {
            throw Exception("First line of source must be empty");
        }
    }

    // Shader type

    const bool hasVS = !source.vsEntry.empty();
    const bool hasFS = !source.fsEntry.empty();
    const bool hasCS = !source.csEntry.empty();
    const bool isGraphicsShader = hasVS && hasFS && !hasCS;
    const bool isComputeShader = !hasVS && !hasFS && hasCS;
    if(!isGraphicsShader && !isComputeShader)
    {
        throw Exception("Invalid shader type. Must be (VS & FS) or (CS)");
    }

    // Parse bindings

    Bindings bindings; bool hasParsedBindings = false;
    auto parseBindings = [&](const std::string &src)
    {
        assert(!hasParsedBindings);
        bindings = CollectBindings(src);
        hasParsedBindings = true;
    };

    if(!hasParsedBindings && !source.vsEntry.empty())
    {
        shaderInfo.entryPoint = source.vsEntry;
        std::string preprocessed;
        dxc.Compile(shaderInfo, DXC::Target::Vulkan_1_3_VS_6_0, debug, &preprocessed);
        parseBindings(preprocessed);
    }
    if(!hasParsedBindings && !source.fsEntry.empty())
    {
        shaderInfo.entryPoint = source.fsEntry;
        std::string preprocessed;
        dxc.Compile(shaderInfo, DXC::Target::Vulkan_1_3_FS_6_0, debug, &preprocessed);
        parseBindings(preprocessed);
    }
    if(!hasParsedBindings && !source.csEntry.empty())
    {
        shaderInfo.entryPoint = source.csEntry;
        std::string preprocessed;
        dxc.Compile(shaderInfo, DXC::Target::Vulkan_1_3_CS_6_0, debug, &preprocessed);
        parseBindings(preprocessed);
    }

    // Regular groups

    std::vector<RC<BindingGroupLayout>> groupLayouts;

    std::map<std::string, std::pair<int, int>> bindingNameToSlots; // name -> (set, slot)
    for(auto &&[groupIndex, group] : Enumerate(bindings.groups))
    {
        BindingGroupLayout::Desc rhiGroupLayoutDesc;
        for(auto &binding : group.bindings)
        {
            BindingGroupLayout::BindingDesc rhiBindingDesc;
            rhiBindingDesc.type = binding.type;
            rhiBindingDesc.stages = binding.stages;
            rhiBindingDesc.arraySize = binding.arraySize;
            rhiGroupLayoutDesc.bindings.push_back(rhiBindingDesc);
        }

        if(!group.valuePropertyDefinitions.empty())
        {
            rhiGroupLayoutDesc.bindings.push_back({ BindingGroupLayout::BindingDesc
            {
                .type = RHI::BindingType::ConstantBuffer,
                .stages = RHI::ShaderStage::All
            }});
            bindingNameToSlots[group.name] = { static_cast<int>(groupIndex), static_cast<int>(group.bindings.size()) };
        }

        for(auto &&[slotIndex, binding] : Enumerate(group.bindings))
        {
            bindingNameToSlots[binding.name] = { static_cast<int>(groupIndex), static_cast<int>(slotIndex) };
        }

        groupLayouts.push_back(device_->CreateBindingGroupLayout(rhiGroupLayoutDesc));
    }

    // Group for inline samplers

    int inlineSamplerGroupIndex = -1;
    if(!bindings.inlineSamplerDescs.empty())
    {
        std::vector<RC<Sampler>> samplers;
        for(auto &s : bindings.inlineSamplerDescs)
        {
            samplers.push_back(device_->CreateSampler(s));
        }
        BindingGroupLayout::Desc groupLayoutDesc;
        groupLayoutDesc.bindings.push_back(BindingGroupLayout::BindingDesc
            {
                .type = RHI::BindingType::Sampler,
                .stages = RHI::ShaderStage::All,
                .arraySize = static_cast<uint32_t>(bindings.inlineSamplerDescs.size()),
                .immutableSamplers = std::move(samplers)
            });
        inlineSamplerGroupIndex = static_cast<int>(groupLayouts.size());
        bindingNameToSlots["_rtrcGeneratedSamplers"] = { inlineSamplerGroupIndex, 0 };
        groupLayouts.push_back(device_->CreateBindingGroupLayout(groupLayoutDesc));
    }

    // Macros

    Macros finalMacros = std::move(shaderInfo.macros);
    finalMacros["rtrc_group(NAME)"] = "_rtrc_group_##NAME";
    finalMacros["rtrc_define(TYPE, NAME, ...)"] = "_rtrc_resource_##NAME";
    finalMacros["rtrc_uniform(A, B)"] = "";
    finalMacros["rtrc_sampler(NAME, ...)"] = "";
    finalMacros["rtrc_ref(NAME, ...)"] = "";
    finalMacros["_rtrc_push_constant_impl(NAME, ...)"] = "_rtrc_push_constant_##NAME";

    for(auto &group : bindings.groups)
    {
        std::string groupLeft = fmt::format("_rtrc_group_{}", group.name);
        std::string groupRight;

        for(auto &&[bindingIndex, binding] : Enumerate(group.bindings))
        {
            const auto [set, slot] = bindingNameToSlots.at(binding.name);

            std::string left = fmt::format("_rtrc_resource_{}", binding.name);
            std::string right = fmt::format(
                "[[vk::binding({}, {})]] {}{} {}{};",
                slot, set,
                ShaderCompilerDetail::BindingTypeToTypeName(binding.type),
                binding.templateParam.empty() ? std::string{} : fmt::format("<{}>", binding.templateParam),
                binding.name,
                binding.arraySize ? fmt::format("[{}]", *binding.arraySize) : "");

            if(group.isRef[bindingIndex])
            {
                finalMacros.insert({ std::move(left), std::move(right) });
            }
            else
            {
                finalMacros.insert({ std::move(left), "" });
                groupRight += right;
            }
        }

        if(!group.valuePropertyDefinitions.empty())
        {
            groupRight += fmt::format(
                "struct _rtrcGeneratedStruct{} {{ {} }}; "
                "[[vk::binding({}, {})]] ConstantBuffer<_rtrcGeneratedStruct{}> {};",
                group.name, group.valuePropertyDefinitions, group.bindings.size(),
                bindings.nameToGroupIndex.at(group.name), group.name, group.name);
        }

        groupRight += fmt::format("struct group_dummy_struct_{}", group.name);
        finalMacros.insert({ std::move(groupLeft), std::move(groupRight) });
    }

    std::string pushConstantContent;
    for(auto &content : bindings.pushConstantRangeContents)
    {
        pushConstantContent += content;
    }
    for(size_t i = 0; i < bindings.pushConstantRanges.size(); ++i)
    {
        const std::string &name = bindings.pushConstantRangeNames[i];

        std::string left = fmt::format("_rtrc_push_constant_{}", name);
        std::string right;
        if(i == 0)
        {
            right = fmt::format(
                "struct rtrc_push_constant_struct {{ {} }}; "
                "[[vk::push_constant]] rtrc_push_constant_struct PushConstant;", pushConstantContent);
        }
        right += fmt::format("struct rtrc_push_constant_dummy_struct_{}", name);
        finalMacros.insert({ std::move(left), std::move(right) });
    }

    std::string generatedShaderPrefix;
    if(inlineSamplerGroupIndex >= 0)
    {
        generatedShaderPrefix += fmt::format(
            "[[vk::binding(0, {})]] SamplerState _rtrcGeneratedSamplers[{}];",
            inlineSamplerGroupIndex, bindings.inlineSamplerDescs.size());
    }
    finalMacros["_rtrcGeneratedPrefix"] = std::move(generatedShaderPrefix);

    for(auto &[name, slot] : bindings.inlineSamplerNameToDescIndex)
    {
        finalMacros.insert({ name, fmt::format("_rtrcGeneratedSamplers[{}]", slot)});
    }

    const int setForUngroupedBindings = static_cast<int>(groupLayouts.size());
    for(auto &binding : bindings.ungroupedBindings)
    {
        const int slot = bindings.ungroupedBindingNameToSlot.at(binding.name);
        std::string left = fmt::format("_rtrc_resource_{}", binding.name);
        std::string right = fmt::format(
            "[[vk::binding({}, {})]] {}{} {}{};",
            slot, setForUngroupedBindings,
            ShaderCompilerDetail::BindingTypeToTypeName(binding.type),
            binding.templateParam.empty() ? std::string{} : fmt::format("<{}>", binding.templateParam),
            binding.name,
            binding.arraySize ? fmt::format("[{}]", *binding.arraySize) : "");
        finalMacros.insert({ std::move(left), std::move(right) });
    }

    // Compile

    shaderInfo.macros = finalMacros;
    shaderInfo.source = "_rtrcGeneratedPrefix" + shaderInfo.source;

    std::vector<uint8_t> vsData, fsData, csData;
    Box<SPIRVReflection> vsRefl, fsRefl, csRefl;
    if(!source.vsEntry.empty())
    {
        shaderInfo.entryPoint = source.vsEntry;
        vsData = dxc.Compile(shaderInfo, DXC::Target::Vulkan_1_3_VS_6_0, debug, nullptr);
        vsRefl = MakeBox<SPIRVReflection>(vsData, source.vsEntry);
    }
    if(!source.fsEntry.empty())
    {
        shaderInfo.entryPoint = source.fsEntry;
        fsData = dxc.Compile(shaderInfo, DXC::Target::Vulkan_1_3_FS_6_0, debug, nullptr);
        fsRefl = MakeBox<SPIRVReflection>(fsData, source.fsEntry);
    }
    if(!source.csEntry.empty())
    {
        shaderInfo.entryPoint = source.csEntry;
        csData = dxc.Compile(shaderInfo, DXC::Target::Vulkan_1_3_CS_6_0, debug, nullptr);
        csRefl = MakeBox<SPIRVReflection>(csData, source.csEntry);
    }

    // Check usage of ungrouped bindings

    auto EnsureAllUsedResourcesAreGrouped = [&](const Box<SPIRVReflection> &refl, std::string_view stageName)
    {
        if(refl)
        {
            for(auto &binding : bindings.ungroupedBindings)
            {
                if(refl->IsBindingUsed(binding.name))
                {
                    throw Exception(fmt::format(
                        "Binding {} is not explicitly grouped but is used in {}", binding.name, stageName));
                }
            }
        }
    };
    EnsureAllUsedResourcesAreGrouped(vsRefl, "vertex shader");
    EnsureAllUsedResourcesAreGrouped(fsRefl, "fragment shader");
    EnsureAllUsedResourcesAreGrouped(csRefl, "compute shader");

    // Shader

    auto shader = MakeRC<Shader>();
    shader->info_ = MakeRC<ShaderInfo>();
    if(!vsData.empty())
    {
        shader->VS_ = device_->GetRawDevice()->CreateShader(
            vsData.data(), vsData.size(), source.vsEntry, RHI::ShaderStage::VertexShader);
        shader->info_->VSInput_ = vsRefl->GetInputVariables();
    }
    if(!fsData.empty())
    {
        shader->FS_ = device_->GetRawDevice()->CreateShader(
            fsData.data(), fsData.size(), source.fsEntry, RHI::ShaderStage::FragmentShader);
    }
    if(!csData.empty())
    {
        shader->CS_ = device_->GetRawDevice()->CreateShader(
            csData.data(), csData.size(), source.csEntry, RHI::ShaderStage::ComputeShader);
    }

    // Constant buffers

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
    if(vsRefl)
    {
        MergeCBuffers(*vsRefl);
    }
    if(fsRefl)
    {
        MergeCBuffers(*fsRefl);
    }
    if(csRefl)
    {
        MergeCBuffers(*csRefl);
    }
    std::ranges::copy(std::ranges::views::values(slotToCBuffer), std::back_inserter(shader->info_->constantBuffers_));

    auto &bindingNameMap = shader->info_->bindingNameMap_;
    bindingNameMap.allBindingNames_.resize(groupLayouts.size());
    for(size_t i = 0; i < groupLayouts.size(); ++i)
    {
        bindingNameMap.allBindingNames_[i].resize(groupLayouts[i]->GetRHIObject()->GetDesc().bindings.size());
    }
    for(auto &pair : bindingNameToSlots)
    {
        bindingNameMap.nameToSetAndSlot_[pair.first] = { pair.second.first, pair.second.second };
        bindingNameMap.allBindingNames_[pair.second.first][pair.second.second] = pair.first;
    }

    shader->info_->nameToBindingGroupLayoutIndex_ = std::move(bindings.nameToGroupIndex);
    shader->info_->bindingGroupLayouts_ = std::move(groupLayouts);
    shader->info_->bindingGroupNames_ = std::move(bindings.groupNames);
    
    BindingLayout::Desc bindingLayoutDesc;
    for(auto &group : shader->info_->bindingGroupLayouts_)
    {
        bindingLayoutDesc.groupLayouts.push_back(group);
    }
    bindingLayoutDesc.pushConstantRanges = bindings.pushConstantRanges;
    shader->info_->bindingLayout_ = device_->CreateBindingLayout(bindingLayoutDesc);

    if(inlineSamplerGroupIndex >= 0)
    {
        shader->info_->bindingGroupForInlineSamplers_ = shader->info_->bindingGroupLayouts_.back()->CreateBindingGroup();
    }

    shader->info_->builtinBindingGroupIndices_[EnumToInt(Shader::BuiltinBindingGroup::Pass)]     = shader->GetBindingGroupIndexByName("Pass");
    shader->info_->builtinBindingGroupIndices_[EnumToInt(Shader::BuiltinBindingGroup::Material)] = shader->GetBindingGroupIndexByName("Material");
    shader->info_->builtinBindingGroupIndices_[EnumToInt(Shader::BuiltinBindingGroup::Object)]   = shader->GetBindingGroupIndexByName("Object");

    if(shader->CS_)
    {
        shader->info_->computeShaderThreadGroupSize_ = csRefl->GetComputeShaderThreadGroupSize();
        shader->computePipeline_ = device_->CreateComputePipeline(shader);
    }

    shader->info_->pushConstantRanges_ = std::move(bindings.pushConstantRanges);

    return shader;
}

std::string ShaderCompiler::MapFilename(std::string_view filename) const
{
    std::filesystem::path path = filename;
    path = path.lexically_normal();
    if(path.is_relative())
    {
        path = rootDir_ / path;
    }
    return absolute(path).lexically_normal().string();
}

template<bool AllowStageSpecifier>
ShaderCompiler::ParsedBinding ShaderCompiler::ParseBinding(ShaderTokenStream &tokens) const
{
    using namespace ShaderCompilerDetail;

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

    RHI::ShaderStageFlag stages = RHI::ShaderStage::All;
    if(tokens.GetCurrentToken() == ",")
    {
        tokens.Next();
        stages = ParseStages(tokens);
    }

    return ParsedBinding
    {
        .name = std::move(bindingName),
        .type = bindingType,
        .arraySize = arraySize,
        .stages = stages,
        .templateParam = std::move(templateParam)
    };
}

void ShaderCompiler::ParseInlineSampler(ShaderTokenStream &tokens, std::string &name, RHI::SamplerDesc &desc) const
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
            if(str == "point")  { return RHI::FilterMode::Point; }
            if(str == "linear") { return RHI::FilterMode::Linear; }
            tokens.Throw(fmt::format("Unknown filter mode: ", str));
        };

        auto TranslateAddressMode = [&](std::string_view str)
        {
            if(str == "repeat") { return RHI::AddressMode::Repeat; }
            if(str == "mirror") { return RHI::AddressMode::Mirror; }
            if(str == "clamp")  { return RHI::AddressMode::Clamp; }
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

void ShaderCompiler::ParsePushConstantRange(
    ShaderTokenStream &tokens, std::string &name, std::string &content,
    Shader::PushConstantRange &range, uint32_t &nextOffset) const
{
    tokens.ConsumeOrThrow("_rtrc_push_constant_impl");
    tokens.ConsumeOrThrow("(");
    name = tokens.GetCurrentToken();
    tokens.Next();
    tokens.ConsumeOrThrow(",");
    if(tokens.GetCurrentToken() != ")")
    {
        range.stages = ShaderCompilerDetail::ParseStages(tokens);
    }
    else
    {
        range.stages = RHI::ShaderStage::All;
    }
    tokens.ConsumeOrThrow(")");
    tokens.ConsumeOrThrow("{");

    bool isFirstMember = true;
    while(true)
    {
        if(tokens.IsFinished())
        {
            tokens.Throw("'}' expected for rtrc_push_constant()");
        }

        if(tokens.GetCurrentToken() == "}")
        {
            break;
        }

        const std::string type = tokens.GetCurrentToken();
        tokens.Next();

        static const std::map<std::string, std::pair<uint32_t, uint32_t>> typeToOffsetAndSize =
        {
            { "float",  { 4,  4 } },
            { "int",    { 4,  4 } },
            { "uint",   { 4,  4 } },
            { "float2", { 8,  8 } },
            { "int2",   { 8,  8 } },
            { "uint2",  { 8,  8 } },
            { "float3", { 16, 12 } },
            { "int3",   { 16, 12 } },
            { "uint3",  { 16, 12 } },
            { "float4", { 16, 16 } },
            { "int4",   { 16, 16 } },
            { "uint4",  { 16, 16 } },
        };

        uint32_t offset, size;
        if(auto it = typeToOffsetAndSize.find(type); it == typeToOffsetAndSize.end())
        {
            tokens.Throw(fmt::format("Unsupported type in push constant block: {}", tokens.GetCurrentToken()));
        }
        else
        {
            std::tie(offset, size) = it->second;
        }

        if(!ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
        {
            tokens.Throw("Push constant variable name expected");
        }
        const std::string varName = tokens.GetCurrentToken();
        tokens.Next();

        std::optional<uint32_t> arraySize;
        if(tokens.GetCurrentToken() == "[")
        {
            tokens.Next();
            try
            {
                arraySize = std::stoul(tokens.GetCurrentToken());
            }
            catch(...)
            {
                tokens.Throw(fmt::format("Invalid array size: {}", tokens.GetCurrentToken()));
            }
            tokens.Next();
            tokens.ConsumeOrThrow("]");
        }

        tokens.ConsumeOrThrow(";");

        nextOffset = UpAlignTo(nextOffset, offset);
        if(isFirstMember)
        {
            isFirstMember = false;
            range.offset = nextOffset;
        }

        content += fmt::format(
            "[[vk::offset({})]] {} {}{};", nextOffset, type, varName,
            arraySize.has_value() ? fmt::format("[{}]", *arraySize) : std::string());
        nextOffset += size * arraySize.value_or(1);
    }

    if(isFirstMember)
    {
        range.offset = nextOffset;
        range.size = 0;
    }
    else
    {
        range.size = nextOffset - range.offset;
    }
}

ShaderCompiler::Bindings ShaderCompiler::CollectBindings(const std::string &source) const
{
    using namespace ShaderCompilerDetail;

    constexpr std::string_view RESOURCE      = "rtrc_define";
    constexpr std::string_view GROUP         = "rtrc_group";
    constexpr std::string_view SAMPLER       = "rtrc_sampler";
    constexpr std::string_view PUSH_CONSTANT = "_rtrc_push_constant_impl";

    std::set<std::string>                parsedBindingNames;
    std::map<std::string, ParsedBinding> ungroupedBindings;

    std::set<std::string>           parsedGroupNames;
    std::vector<ParsedBindingGroup> parsedGroups;

    std::vector<RHI::SamplerDesc>   inlineSamplerDescs;
    std::map<std::string, int>      inlineSamplerNameToDescIndex;
    std::map<RHI::SamplerDesc, int> inlineSamplerDescToIndex;

    std::vector<Shader::PushConstantRange> pushConstantRanges;
    std::vector<std::string>               pushConstantRangeContents;
    std::vector<std::string>               pushConstantRangeNames;
    uint32_t                               pushConstantRangeOffset = 0;

    size_t keywordBeginPos = 0;
    while(true)
    {
        const size_t resourcePos     = FindKeyword(source, RESOURCE, keywordBeginPos);
        const size_t groupPos        = FindKeyword(source, GROUP, keywordBeginPos);
        const size_t samplerPos      = FindKeyword(source, SAMPLER, keywordBeginPos);
        const size_t pushConstantPos = FindKeyword(source, PUSH_CONSTANT, keywordBeginPos);
        const size_t minPos = (std::min)({ resourcePos, groupPos, samplerPos, pushConstantPos });

        if(minPos == std::string::npos)
        {
            break;
        }

        if(minPos == resourcePos)
        {
            keywordBeginPos = resourcePos + RESOURCE.size();
            ShaderTokenStream tokens(source, keywordBeginPos);
            tokens.ConsumeOrThrow("(");
            ParsedBinding binding = ParseBinding<false>(tokens);
            if(parsedBindingNames.contains(binding.name))
            {
                tokens.Throw(fmt::format("Binding {} is defined multiple times", binding.name));
            }
            parsedBindingNames.insert(binding.name);
            ungroupedBindings.insert({ binding.name, binding });
            tokens.ConsumeOrThrow(")");
            continue;
        }

        if(minPos == samplerPos)
        {
            keywordBeginPos = samplerPos + SAMPLER.size();
            ShaderTokenStream tokens(source, keywordBeginPos);
            tokens.ConsumeOrThrow("(");
            std::string name; RHI::SamplerDesc desc;
            ParseInlineSampler(tokens, name, desc);
            tokens.ConsumeOrThrow(")");

            if(auto it = inlineSamplerNameToDescIndex.find(name); it != inlineSamplerNameToDescIndex.end())
            {
                throw Exception(fmt::format("Sampler {} is defined multiple times", name));
            }

            if(auto it = inlineSamplerDescToIndex.find(desc); it != inlineSamplerDescToIndex.end())
            {
                inlineSamplerNameToDescIndex[name] = it->second;
                continue;
            }

            const int newDescIndex = static_cast<int>(inlineSamplerDescs.size());
            inlineSamplerNameToDescIndex[name] = newDescIndex;
            inlineSamplerDescToIndex[desc] = newDescIndex;
            inlineSamplerDescs.push_back(desc);
            continue;
        }

        if(minPos == pushConstantPos)
        {
            keywordBeginPos = pushConstantPos + PUSH_CONSTANT.size();
            ShaderTokenStream tokens(source, pushConstantPos);
            ParsePushConstantRange(
                tokens, pushConstantRangeNames.emplace_back(), pushConstantRangeContents.emplace_back(),
                pushConstantRanges.emplace_back(), pushConstantRangeOffset);
            continue;
        }

        ShaderTokenStream tokens(source, groupPos + GROUP.size());

        tokens.ConsumeOrThrow("(");
        std::string groupName = tokens.GetCurrentToken();
        if(!ShaderTokenStream::IsIdentifier(groupName))
        {
            tokens.Throw("Group name expected");
        }
        tokens.Next();
        tokens.ConsumeOrThrow(")");
        tokens.ConsumeOrThrow("{");
        if(parsedGroupNames.contains(groupName))
        {
            tokens.Throw(fmt::format("Group {} is already defined", groupName));
        }
        parsedGroupNames.insert(groupName);

        ParsedBindingGroup &currentGroup = parsedGroups.emplace_back();
        currentGroup.name = std::move(groupName);

        while(true)
        {
            if(tokens.GetCurrentToken() == RESOURCE)
            {
                tokens.Next();
                tokens.ConsumeOrThrow("(");
                if(RESOURCE_PROPERTIES.contains(tokens.GetCurrentToken()))
                {
                    currentGroup.bindings.push_back(ParseBinding<true>(tokens));
                    currentGroup.isRef.push_back(false);
                    const std::string &bindingName = currentGroup.bindings.back().name;
                    if(parsedBindingNames.contains(bindingName))
                    {
                        tokens.Throw(fmt::format("Resource {} is defined multiple times", bindingName));
                    }
                    parsedBindingNames.insert(bindingName);
                }
                else
                {
                    tokens.Throw(fmt::format("Unknown resource type: {}", tokens.GetCurrentToken()));
                }
                tokens.ConsumeOrThrow(")");
            }
            else if(tokens.GetCurrentToken() == "rtrc_ref")
            {
                tokens.Next();
                tokens.ConsumeOrThrow("(");

                if(!ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
                {
                    tokens.Throw(fmt::format("Invalid binding name: {}", tokens.GetCurrentToken()));
                }

                auto it = ungroupedBindings.find(tokens.GetCurrentToken());
                if(it == ungroupedBindings.end())
                {
                    tokens.Throw(fmt::format("{} was not defined as a resource binding", tokens.GetCurrentToken()));
                }

                currentGroup.bindings.push_back(it->second);
                currentGroup.isRef.push_back(true);
                ungroupedBindings.erase(it);
                tokens.Next();

                if(tokens.GetCurrentToken() == ",")
                {
                    tokens.Next();
                    currentGroup.bindings.back().stages = ParseStages(tokens);
                }
                else
                {
                    currentGroup.bindings.back().stages = RHI::ShaderStage::All;
                }

                tokens.ConsumeOrThrow(")");
            }
            else if(tokens.GetCurrentToken() == "rtrc_uniform")
            {
                tokens.Next();
                tokens.ConsumeOrThrow("(");
                //if(!VALUE_PROPERTIES.contains(tokens.GetCurrentToken()))
                //{
                //    tokens.Throw(fmt::format("Unknown value property type: {}", tokens.GetCurrentToken()));
                //}
                currentGroup.valuePropertyDefinitions += tokens.GetCurrentToken();
                tokens.Next();
                while(tokens.GetCurrentToken() == "::")
                {
                    tokens.Next();
                    currentGroup.valuePropertyDefinitions += "::" + tokens.GetCurrentToken();
                    tokens.Next();
                }
                tokens.ConsumeOrThrow(",");
                if(!ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
                {
                    tokens.Throw("Uniform property name expected");
                }
                currentGroup.valuePropertyDefinitions += fmt::format(" {};", tokens.GetCurrentToken());
                tokens.Next();
                tokens.ConsumeOrThrow(")");
            }
            else if(tokens.GetCurrentToken() == "}")
            {
                keywordBeginPos = tokens.GetCurrentPosition();
                break;
            }
            else
            {
                tokens.Throw("'rtrc_define', 'rtrc_uniform' or '}' expected");
            }
        }
    }

    Bindings bindings;
    bindings.groups = std::move(parsedGroups);
    for(auto &&[index, group] : Enumerate(bindings.groups))
    {
        bindings.groupNames.push_back(group.name);
        bindings.nameToGroupIndex[group.name] = static_cast<int>(index);
    }
    for(auto &&[index, binding] : Enumerate(std::ranges::views::values(ungroupedBindings)))
    {
        bindings.ungroupedBindings.push_back(binding);
        bindings.ungroupedBindingNameToSlot[binding.name] = static_cast<int>(index);
    }
    bindings.inlineSamplerDescs = std::move(inlineSamplerDescs);
    bindings.inlineSamplerNameToDescIndex = std::move(inlineSamplerNameToDescIndex);

    bindings.pushConstantRanges = std::move(pushConstantRanges);
    bindings.pushConstantRangeContents = std::move(pushConstantRangeContents);
    bindings.pushConstantRangeNames = std::move(pushConstantRangeNames);

    return bindings;
}

RTRC_END
