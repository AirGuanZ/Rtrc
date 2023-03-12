#include <ranges>
#include <stack>

#include <Rtrc/Graphics/Shader/DXC.h>
#include <Rtrc/Graphics/Shader/ShaderCompiler.h>
#include <Rtrc/Graphics/Shader/ShaderTokenStream.h>
#include <Rtrc/Utility/Enumerate.h>
#include <Rtrc/Utility/Filesystem/File.h>
#include <Rtrc/Utility/String.h>

RTRC_BEGIN

namespace ShaderCompilerDetail
{

    const char *BindingTypeToTypeName(RHI::BindingType type)
    {
        static const char *NAMES[] =
        {
            "Texture",
            "RWTexture",
            "Buffer",
            "StructuredBuffer",
            "RWBuffer",
            "RWStructuredBuffer",
            "ConstantBuffer",
            "SamplerState",
            "RaytracingAccelerationStructure"
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
        { "ConstantBuffer",                  RHI::BindingType::ConstantBuffer },
        { "SamplerState",                    RHI::BindingType::Sampler },
        { "RaytracingAccelerationStructure", RHI::BindingType::AccelerationStructure }
    };

    RHI::ShaderStageFlags ParseStages(ShaderTokenStream &tokens)
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
            else if(tokens.GetCurrentToken() == "Graphics")
            {
                stages |= RHI::ShaderStage::AllGraphics;
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

    RHI::ShaderStageFlags ShaderCategoryToStages(Shader::Category category)
    {
        RHI::ShaderStageFlags stages = RHI::ShaderStage::All;
        switch(category)
        {
        case Shader::Category::Graphics:   stages = RHI::ShaderStage::AllGraphics; break;
        case Shader::Category::Compute:    stages = RHI::ShaderStage::CS;          break;
        case Shader::Category::RayTracing: stages = RHI::ShaderStage::AllRT;       break;
        }
        return stages;
    }

    // Parse '#entryName args...'
    bool ParseAndErasePragma(std::string &source, std::string_view entryName, std::vector<std::string> &args)
    {
        args.clear();
        
        size_t beginPos = 0;
        while(true)
        {
            const size_t pos = source.find(entryName, beginPos);
            if(pos == std::string::npos)
            {
                return false;
            }

            const size_t findLen = entryName.size();
            if(!ShaderTokenStream::IsNonIdentifierChar(source[pos + findLen]))
            {
                beginPos = pos + findLen;
                continue;
            }

            size_t endPos = source.find('\n', pos + findLen);
            if(endPos == std::string::npos)
            {
                endPos = source.size();
            }

            // Args

            ShaderTokenStream tokens(source.substr(pos + findLen, endPos - (pos + findLen)), 0);
            while(!tokens.IsFinished())
            {
                args.push_back(tokens.GetCurrentToken());
                tokens.Next();
            }

            // Erase

            for(size_t i = pos; i < endPos; ++i)
            {
                source[i] = ' ';
            }

            return true;
        }
    }

} // namespace ShaderCompilerDetail

void ShaderCompiler::SetDevice(Device *device)
{
    device_ = device;
}

void ShaderCompiler::AddIncludeDirectory(std::string_view dir)
{
    includeDirs_.insert(std::filesystem::absolute(dir).lexically_normal().string());
}

RC<Shader> ShaderCompiler::Compile(const ShaderSource &source, const Macros &macros, bool debug) const
{
    std::string actualSource;
    if(source.source.empty())
    {
        assert(!source.filename.empty());
        actualSource = File::ReadTextFile(source.filename);
    }
    else
    {
        actualSource = source.source;
    }
    
    const ParsedShaderEntry shaderEntry = ParseShaderEntry(actualSource);

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
        .source = std::move(actualSource),
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

    // Shader category

    const bool hasVS = !shaderEntry.vertexEntry.empty();
    const bool hasFS = !shaderEntry.fragmentEntry.empty();
    const bool hasCS = !shaderEntry.computeEntry.empty();
    const bool hasRT = shaderEntry.isRayTracingShader;

    Shader::Category category;
    if(hasVS && hasFS && !hasCS && !hasRT)
    {
        category = Shader::Category::Graphics;
    }
    else if(!hasVS && !hasFS && hasCS && !hasRT)
    {
        category = Shader::Category::Compute;
    }
    else if(!hasVS && !hasFS && !hasCS && hasRT)
    {
        category = Shader::Category::RayTracing;
    }
    else
    {
        throw Exception("Invalid shader category. Must be (VS & FS), (CS) or (RT)");
    }

    // Parse bindings

    Bindings bindings; bool hasParsedBindings = false;
    auto GetBindings = [&](const std::string &src)
    {
        assert(!hasParsedBindings);
        bindings = CollectBindings(src, category);
        hasParsedBindings = true;
    };

    if(!hasParsedBindings && hasVS)
    {
        shaderInfo.entryPoint = shaderEntry.vertexEntry;
        std::string preprocessed;
        dxc.Compile(shaderInfo, DXC::Target::Vulkan_1_3_VS_6_0, debug, &preprocessed);
        GetBindings(preprocessed);
    }
    if(!hasParsedBindings && hasFS)
    {
        shaderInfo.entryPoint = shaderEntry.fragmentEntry;
        std::string preprocessed;
        dxc.Compile(shaderInfo, DXC::Target::Vulkan_1_3_FS_6_0, debug, &preprocessed);
        GetBindings(preprocessed);
    }
    if(!hasParsedBindings && hasCS)
    {
        shaderInfo.entryPoint = shaderEntry.computeEntry;
        std::string preprocessed;
        dxc.Compile(shaderInfo, DXC::Target::Vulkan_1_3_CS_6_0, debug, &preprocessed);
        GetBindings(preprocessed);
    }
    if(!hasParsedBindings && hasRT)
    {
        std::string preprocessed;
        dxc.Compile(shaderInfo, DXC::Target::Vulkan_1_3_RT_6_4, debug, &preprocessed);
        GetBindings(preprocessed);
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
            rhiBindingDesc.type      = binding.type;
            rhiBindingDesc.stages    = binding.stages;
            rhiBindingDesc.arraySize = binding.arraySize;
            rhiBindingDesc.bindless  = binding.bindless;
            rhiGroupLayoutDesc.bindings.push_back(rhiBindingDesc);

            if(binding.variableArraySize)
            {
                if(&binding != &group.bindings.back())
                {
                    throw Exception(fmt::format(
                        "Binding {} has variable array size but is not the last one in binding group {}",
                        binding.name, group.name));
                }
                rhiGroupLayoutDesc.variableArraySize = true;
            }
        }

        if(!group.uniformPropertyDefinitions.empty())
        {
            rhiGroupLayoutDesc.bindings.push_back({ BindingGroupLayout::BindingDesc
            {
                .type = RHI::BindingType::ConstantBuffer,
                .stages = group.defaultStages
            }});
            bindingNameToSlots[group.name] = { static_cast<int>(groupIndex), static_cast<int>(group.bindings.size()) };
        }

        for(auto &&[slotIndex, binding] : Enumerate(group.bindings))
        {
            bindingNameToSlots[binding.name] = { static_cast<int>(groupIndex), static_cast<int>(slotIndex) };
        }

        // Binding with variable array size must be the last one in group
        // In that case, swap the slots of variable-sized binding and uniform-variable binding
        if(!group.uniformPropertyDefinitions.empty() && !group.bindings.empty() && group.bindings.back().variableArraySize)
        {
            const size_t a = rhiGroupLayoutDesc.bindings.size() - 2;
            const size_t b = rhiGroupLayoutDesc.bindings.size() - 1;
            std::swap(rhiGroupLayoutDesc.bindings[a], rhiGroupLayoutDesc.bindings[b]);
            std::swap(bindingNameToSlots[group.name], bindingNameToSlots[group.bindings.back().name]);
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
                .stages = ShaderCompilerDetail::ShaderCategoryToStages(category),
                .arraySize = static_cast<uint32_t>(bindings.inlineSamplerDescs.size()),
                .immutableSamplers = std::move(samplers)
            });
        inlineSamplerGroupIndex = static_cast<int>(groupLayouts.size());
        bindingNameToSlots["_rtrcGeneratedSamplers"] = { inlineSamplerGroupIndex, 0 };
        groupLayouts.push_back(device_->CreateBindingGroupLayout(groupLayoutDesc));
    }

    // Macros

    // Binding group transformation:
    //
    //      rtrc_define(Texture2D<float4>, TexA)
    //      rtrc_group(GroupName)
    //      {
    //          rtrc_ref(TexA)
    //          rtrc_define(Texture2D<float3>, TexB)
    //          rtrc_uniform(float3, xyz)
    //          rtrc_uniform(float3, w)
    //      };
    // ===>
    //      _rtrc_resource_TexA
    //      _rtrc_group_GroupName
    //      {
    //
    //          _rtrc_resource_TexB;
    //
    //          
    //      };
    // where:
    //      _rtrc_resource_TexA expands to:
    //          [[vk::binding(0, 0)]] Texture2D<float4> TexA;
    //      _rtrc_group_GroupName expands to:
    //          [[vk::binding(1, 0)]] Texture2D<float3> TexB;
    //          struct _rtrc_generated_cbuffer_struct_GroupName
    //          {
    //              float3 xyz;
    //              float w;
    //          };
    //          [[vk::binding(2, 0)]] ConstantBuffer<_rtrc_generated_cbuffer_struct_GroupName> GroupName;
    //          struct group_dummy_struct_GroupName
    //      and _rtrc_resource_TexB expands to nothing

    // Inline sampler transformation:
    //
    //      rtrc_sampler(SamplerA, filter = point, address = clamp)
    //      rtrc_sampler(SamplerB, filter = linear, address_u = clamp, address_v = repeat)
    //  ===>
    //      _rtrc_sampler_SamplerA
    //      _rtrc_sampler_SamplerB
    //  ===>
    //      static SamplerState SamplerA = _rtrc_generated_samplers[0];
    //      static SamplerState SamplerB = _rtrc_generated_samplers[1];
    //  where _rtrc_generated_samplers is defined in _rtrc_generated_shader_prefix, which is put at the shader begining:
    //      [[vk::binding(0, x)]] SamplerState _rtrc_generated_samplers[2]; // x = last_binding_group_index + 1

    // Push constant block transformation:
    //
    //      rtrc_push_constant(VS)
    //      {
    //      	float3 position;
    //      };
    //      rtrc_push_constant(FS)
    //      {
    //      	float3 albedo;
    //      };
    //      ===>
    //      _rtrc_push_constant_0 // Note that blocker indices 0 and 1 are generated with builtin macro __COUNTER__
    //      {
    //      	float3 offset;
    //      };
    //      _rtrc_push_constant_1
    //      {
    //      	float3 albedo;
    //      };
    // where:
    //      _rtrc_push_constant_0 expands to:
    //          struct _rtrc_push_constant_struct
    //          {
    //          	[[vk::offset(0)]] float3 position;
    //          	[[vk::offset(16)]] float3 albedo;
    //          };
    //          struct _rtrc_push_constant_dummy_struct_0
    //      _rtrc_push_constant_1 expands to:
    //          struct _rtrc_push_constant_dummy_struct_1

    Macros finalMacros = std::move(shaderInfo.macros);
    finalMacros["rtrc_group(NAME, ...)"]                  = "_rtrc_group_##NAME";
    finalMacros["rtrc_define(TYPE, NAME, ...)"]           = "_rtrc_resource_##NAME";
    finalMacros["rtrc_alias(TYPE, NAME, ...)"]            = "_rtrc_alias_##NAME";
    finalMacros["rtrc_bindless(TYPE, NAME, ...)"]         = "_rtrc_resource_##NAME";
    finalMacros["rtrc_bindless_varsize(TYPE, NAME, ...)"] = "_rtrc_resource_##NAME";
    finalMacros["rtrc_uniform(A, B)"]                     = "";
    finalMacros["rtrc_sampler(NAME, ...)"]                = "_rtrc_sampler_##NAME";
    finalMacros["rtrc_ref(NAME, ...)"]                    = "";

    // rtrc_push_constant(...) is replaced with _rtrc_push_constant_impl(value of __COUNTER__, ...) in preprocessing
    finalMacros["_rtrc_push_constant_impl(NAME, ...)"] = "_rtrc_push_constant_##NAME";

    bool hasBindless = false;
    std::map<std::string, std::string> bindingNameToArraySpecifier;

    for(auto &group : bindings.groups)
    {
        std::string groupLeft = fmt::format("_rtrc_group_{}", group.name);
        std::string groupRight;

        for(auto &&[bindingIndex, binding] : Enumerate(group.bindings))
        {
            const auto [set, slot] = bindingNameToSlots.at(binding.name);

            std::string arraySpecifier = binding.GetArraySpeficier();
            bindingNameToArraySpecifier[binding.name] = arraySpecifier;

            std::string left = fmt::format("_rtrc_resource_{}", binding.name);
            std::string right = fmt::format(
                "[[vk::binding({}, {})]] {}{} {}{};",
                slot, set,
                binding.rawTypename,
                binding.templateParam.empty() ? std::string{} : fmt::format("<{}>", binding.templateParam),
                binding.name, std::move(arraySpecifier));

            if(group.isRef[bindingIndex])
            {
                finalMacros.insert({ std::move(left), std::move(right) });
            }
            else
            {
                finalMacros.insert({ std::move(left), "" });
                groupRight += right;
            }

            hasBindless |= binding.bindless;
        }

        if(!group.uniformPropertyDefinitions.empty())
        {
            std::string uniformPropertiesStr;
            for(const ParsedUniformDefinition &definition : group.uniformPropertyDefinitions)
            {
                uniformPropertiesStr += fmt::format("{} {};", definition.type, definition.name);
            }

            const auto [set, slot] = bindingNameToSlots.at(group.name);
            groupRight += fmt::format(
                "struct _rtrc_generated_cbuffer_struct_{} {{ {} }}; "
                "[[vk::binding({}, {})]] ConstantBuffer<_rtrc_generated_cbuffer_struct_{}> {};",
                group.name, uniformPropertiesStr,
                slot, set, group.name, group.name);
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
                "struct _rtrc_push_constant_struct {{ {} }}; "
                "[[vk::push_constant]] _rtrc_push_constant_struct PushConstant;", pushConstantContent);
        }
        right += fmt::format("struct _rtrc_push_constant_dummy_struct_{}", name);
        finalMacros.insert({ std::move(left), std::move(right) });
    }

    std::string generatedShaderPrefix;
    if(inlineSamplerGroupIndex >= 0)
    {
        generatedShaderPrefix += fmt::format(
            "[[vk::binding(0, {})]] SamplerState _rtrc_generated_samplers[{}];",
            inlineSamplerGroupIndex, bindings.inlineSamplerDescs.size());
    }
    finalMacros["_rtrc_generated_shader_prefix"] += std::move(generatedShaderPrefix);

    for(auto &[name, slot] : bindings.inlineSamplerNameToDescIndex)
    {
        std::string left = fmt::format("_rtrc_sampler_{}", name);
        std::string right = fmt::format("static SamplerState {} = _rtrc_generated_samplers[{}];", name, slot);
        finalMacros.insert({ std::move(left), std::move(right) });
    }

    const int setForUngroupedBindings = static_cast<int>(groupLayouts.size());
    for(auto &binding : bindings.ungroupedBindings)
    {
        const int slot = bindings.ungroupedBindingNameToSlot.at(binding.name);
        std::string arraySpecifier = binding.GetArraySpeficier();
        std::string left = fmt::format("_rtrc_resource_{}", binding.name);
        std::string right = fmt::format(
            "[[vk::binding({}, {})]] {}{} {}{};",
            slot, setForUngroupedBindings,
            binding.rawTypename,
            binding.templateParam.empty() ? std::string{} : fmt::format("<{}>", binding.templateParam),
            binding.name, arraySpecifier);
        bindingNameToArraySpecifier[binding.name] = std::move(arraySpecifier);
        finalMacros.insert({ std::move(left), std::move(right) });
    }

    for(const ParsedBindingAlias &alias : bindings.aliases)
    {
        int set, slot;
        if(auto it = bindingNameToSlots.find(alias.aliasedName); it != bindingNameToSlots.end())
        {
            set = it->second.first;
            slot = it->second.second;
        }
        else if(auto jt = bindings.ungroupedBindingNameToSlot.find(alias.aliasedName);
                jt != bindings.ungroupedBindingNameToSlot.end())
        {
            set = setForUngroupedBindings;
            slot = jt->second;
        }
        else
        {
            throw Exception(fmt::format("Unknown aliased name: {}", alias.aliasedName));
        }

        std::string left = fmt::format("_rtrc_alias_{}", alias.name);
        std::string right = fmt::format(
            "[[vk::binding({}, {})]] {}{} {}{};",
            slot, set, alias.rawTypename,
            alias.templateParam.empty() ? std::string{} : fmt::format("<{}>", alias.templateParam),
            alias.name, bindingNameToArraySpecifier.at(alias.aliasedName));
        finalMacros.insert({ std::move(left), std::move(right) });
    }

    // Compile

    shaderInfo.macros   = finalMacros;
    shaderInfo.source   = "_rtrc_generated_shader_prefix" + shaderInfo.source;
    shaderInfo.bindless = hasBindless;

    if(hasRT)
    {
        shaderInfo.rayTracing = true;
        shaderInfo.rayQuery = true;
    }
    else
    {
        auto HasASBinding = [&]
        {
            for(auto &group : bindings.groups)
            {
                for(auto &binding : group.bindings)
                {
                    if(binding.type == RHI::BindingType::AccelerationStructure)
                    {
                        return true;
                    }
                }
            }
            return false;
        };
        shaderInfo.rayQuery = HasASBinding();
    }

    std::vector<uint8_t> vsData, fsData, csData, rtData;
    Box<SPIRVReflection> vsRefl, fsRefl, csRefl, rtRefl;
    
    if(hasVS)
    {
        shaderInfo.entryPoint = shaderEntry.vertexEntry;
        vsData = dxc.Compile(shaderInfo, DXC::Target::Vulkan_1_3_VS_6_0, debug, nullptr);
        vsRefl = MakeBox<SPIRVReflection>(vsData, shaderEntry.vertexEntry);
    }
    if(hasFS)
    {
        shaderInfo.entryPoint = shaderEntry.fragmentEntry;
        fsData = dxc.Compile(shaderInfo, DXC::Target::Vulkan_1_3_FS_6_0, debug, nullptr);
        fsRefl = MakeBox<SPIRVReflection>(fsData, shaderEntry.fragmentEntry);
    }
    if(hasCS)
    {
        shaderInfo.entryPoint = shaderEntry.computeEntry;
        csData = dxc.Compile(shaderInfo, DXC::Target::Vulkan_1_3_CS_6_0, debug, nullptr);
        csRefl = MakeBox<SPIRVReflection>(csData, shaderEntry.computeEntry);
    }
    if(hasRT)
    {
        rtData = dxc.Compile(shaderInfo, DXC::Target::Vulkan_1_3_RT_6_4, debug, nullptr);
        rtRefl = MakeBox<SPIRVReflection>(rtData, std::string{});
    }

    // Check uses of ungrouped bindings

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
    EnsureAllUsedResourcesAreGrouped(rtRefl, "ray tracing shader");

    // Shader

    auto shader = MakeRC<Shader>();
    shader->info_ = MakeRC<ShaderInfo>();
    shader->info_->shaderBindingLayoutInfo_ = MakeRC<ShaderBindingLayoutInfo>();
    shader->info_->category_ = category;
    if(hasVS)
    {
        shader->rawShaders_[Shader::VS_INDEX] = device_->GetRawDevice()->CreateShader(
            vsData.data(), vsData.size(), { { RHI::ShaderStage::VertexShader, shaderEntry.vertexEntry } });
        shader->info_->VSInput_ = vsRefl->GetInputVariables();
    }
    if(hasFS)
    {
        shader->rawShaders_[Shader::FS_INDEX] = device_->GetRawDevice()->CreateShader(
            fsData.data(), fsData.size(), { { RHI::ShaderStage::FragmentShader, shaderEntry.fragmentEntry } });
    }
    if(hasCS)
    {
        shader->rawShaders_[Shader::CS_INDEX] = device_->GetRawDevice()->CreateShader(
            csData.data(), csData.size(), { { RHI::ShaderStage::ComputeShader, shaderEntry.computeEntry } });
    }
    if(hasRT)
    {
        std::vector<RHI::RawShaderEntry> rayTracingEntries = rtRefl->GetEntries();

        auto FindEntryIndex = [&](std::string_view name)
        {
            for(size_t i = 0; i < rayTracingEntries.size(); ++i)
            {
                if(rayTracingEntries[i].name == name)
                {
                    return static_cast<uint32_t>(i);
                }
            }
            throw Exception(fmt::format(
            "Ray tracing entry {} is required by a shader group but not defined", name));
        };

        auto shaderGroups = MakeRC<ShaderGroupInfo>();
        for(const std::vector<std::string> &rawGroup : shaderEntry.shaderGroups)
        {
            assert(!rawGroup.empty());
            const uint32_t firstEntryIndex = FindEntryIndex(rawGroup[0]);
            if(rayTracingEntries[firstEntryIndex].stage == RHI::ShaderStage::RT_RayGenShader)
            {
                if(rawGroup.size() != 1)
                {
                    throw Exception("RayGen shader group must contains no more than one shader");
                }
                shaderGroups->rayGenShaderGroups_.push_back({ firstEntryIndex });
            }
            else if(rayTracingEntries[firstEntryIndex].stage == RHI::ShaderStage::RT_MissShader)
            {
                if(rawGroup.size() != 1)
                {
                    throw Exception("Miss shader group must contains no more than one shader");
                }
                shaderGroups->missShaderGroups_.push_back({ firstEntryIndex });
            }
            else
            {
                RHI::RayTracingHitShaderGroup group;
                auto HandleEntryIndex = [&](uint32_t index)
                {
                    const RHI::ShaderStage stage = rayTracingEntries[index].stage;
                    if(stage == RHI::ShaderStage::RT_ClosestHitShader)
                    {
                        if(group.closestHitShaderIndex != RHI::RAY_TRACING_UNUSED_SHADER)
                        {
                            throw Exception("Closest hit entry is already specified ");
                        }
                        group.closestHitShaderIndex = index;
                    }
                    else if(stage == RHI::ShaderStage::RT_AnyHitShader)
                    {
                        if(group.anyHitShaderIndex != RHI::RAY_TRACING_UNUSED_SHADER)
                        {
                            throw Exception("Any hit entry is already specified ");
                        }
                        group.anyHitShaderIndex = index;
                    }
                    else if(stage == RHI::ShaderStage::RT_IntersectionShader)
                    {
                        if(group.intersectionShaderIndex != RHI::RAY_TRACING_UNUSED_SHADER)
                        {
                            throw Exception("Intersection entry is already specified ");
                        }
                        group.intersectionShaderIndex = index;
                    }
                    else
                    {
                        throw Exception(fmt::format(
                            "Unsupported shader stage in ray tracing shader group: {}", std::to_underlying(stage)));
                    }
                };

                HandleEntryIndex(firstEntryIndex);
                for(size_t i = 1; i < rawGroup.size(); ++i)
                {
                    const uint32_t entryIndex = FindEntryIndex(rawGroup[i]);
                    HandleEntryIndex(entryIndex);
                }

                if(!group.closestHitShaderIndex)
                {
                    throw Exception("One must raygen/miss/closesthit shader must be on a shader group");
                }
                shaderGroups->hitShaderGroups_.push_back(group);
            }
        }

        shader->info_->shaderGroupInfo_ = std::move(shaderGroups);
        shader->rawShaders_[Shader::RT_INDEX] = device_->GetRawDevice()->CreateShader(
            rtData.data(), rtData.size(), std::move(rayTracingEntries));
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
    if(hasVS)
    {
        MergeCBuffers(*vsRefl);
    }
    if(hasFS)
    {
        MergeCBuffers(*fsRefl);
    }
    if(hasCS)
    {
        MergeCBuffers(*csRefl);
    }
    if(hasRT)
    {
        MergeCBuffers(*rtRefl);
    }
    std::ranges::copy(std::ranges::views::values(slotToCBuffer), std::back_inserter(shader->info_->constantBuffers_));

    auto &bindingNameMap = shader->info_->shaderBindingLayoutInfo_->bindingNameMap_;
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

    shader->info_->shaderBindingLayoutInfo_->nameToBindingGroupLayoutIndex_ = std::move(bindings.nameToGroupIndex);
    shader->info_->shaderBindingLayoutInfo_->bindingGroupLayouts_ = std::move(groupLayouts);
    shader->info_->shaderBindingLayoutInfo_->bindingGroupNames_ = std::move(bindings.groupNames);
    
    BindingLayout::Desc bindingLayoutDesc;
    for(auto &group : shader->info_->shaderBindingLayoutInfo_->bindingGroupLayouts_)
    {
        bindingLayoutDesc.groupLayouts.push_back(group);
    }
    bindingLayoutDesc.pushConstantRanges = bindings.pushConstantRanges;
    shader->info_->shaderBindingLayoutInfo_->bindingLayout_ = device_->CreateBindingLayout(bindingLayoutDesc);

    if(inlineSamplerGroupIndex >= 0)
    {
        shader->info_->shaderBindingLayoutInfo_->bindingGroupForInlineSamplers_ =
            shader->info_->shaderBindingLayoutInfo_->bindingGroupLayouts_.back()->CreateBindingGroup();
    }

    auto SetBuiltinBindingGroupIndex = [&](Shader::BuiltinBindingGroup group, std::string_view name)
    {
        shader->info_->shaderBindingLayoutInfo_->builtinBindingGroupIndices_[std::to_underlying(group)] =
            shader->GetBindingGroupIndexByName(name);
    };
    SetBuiltinBindingGroupIndex(Shader::BuiltinBindingGroup::Pass,     "Pass");
    SetBuiltinBindingGroupIndex(Shader::BuiltinBindingGroup::Material, "Material");
    SetBuiltinBindingGroupIndex(Shader::BuiltinBindingGroup::Object,   "Object");

    if(hasCS)
    {
        shader->info_->computeShaderThreadGroupSize_ = csRefl->GetComputeShaderThreadGroupSize();
        shader->computePipeline_ = device_->CreateComputePipeline(shader);
    }

    shader->info_->shaderBindingLayoutInfo_->pushConstantRanges_ = std::move(bindings.pushConstantRanges);

    return shader;
}

std::string ShaderCompiler::ParsedBinding::GetArraySpeficier() const
{
    if(variableArraySize)
    {
        assert(arraySize.has_value());
        return "[]";
    }
    if(arraySize.has_value())
    {
        return fmt::format("[{}]", arraySize.value());
    }
    return {};
}

ShaderCompiler::ParsedShaderEntry ShaderCompiler::ParseShaderEntry(std::string &source) const
{
    ParsedShaderEntry ret;

    auto ParseEntry = [&](std::string_view pragma, std::string &output)
    {
        std::vector<std::string> args;
        if(!ShaderCompilerDetail::ParseAndErasePragma(source, pragma, args))
        {
            return;
        }
        if(args.size() != 1)
        {
            throw Exception(fmt::format("#pragma {} expects 1 argument", pragma));
        }
        if(!ShaderTokenStream::IsIdentifier(args[0]))
        {
            throw Exception(fmt::format("Invalid shader entry name {}", args[0]));
        }
        if(!output.empty())
        {
            throw Exception(fmt::format("Shader entry of {} is specified again ({})", pragma, args[0]));
        }
        output = args[0];
    };

    ParseEntry("#vertex",   ret.vertexEntry);
    ParseEntry("#vert",     ret.vertexEntry);
    ParseEntry("#fragment", ret.fragmentEntry);
    ParseEntry("#frag",     ret.fragmentEntry);
    ParseEntry("#pixel",    ret.fragmentEntry);
    ParseEntry("#compute",  ret.computeEntry);
    ParseEntry("#comp",     ret.computeEntry);

    std::vector<std::string> args;
    if(ShaderCompilerDetail::ParseAndErasePragma(source, "#raytracing", args) ||
       ShaderCompilerDetail::ParseAndErasePragma(source, "#rt", args))
    {
        if(!args.empty())
        {
            throw Exception("#raytracing/rt shouldn't have any argument");
        }
        ret.isRayTracingShader = true;
    }

    while(ShaderCompilerDetail::ParseAndErasePragma(source, "#group", args))
    {
        if(args.empty() || args.size() > 3)
        {
            throw Exception(fmt::format("Invalid #group argument count: {}", args.size()));
        }
        auto &group = ret.shaderGroups.emplace_back();
        group = std::move(args);
    }

    return ret;
}

template<bool AllowStageSpecifier, ShaderCompiler::BindingCategory Category>
ShaderCompiler::ParsedBinding ShaderCompiler::ParseBinding(
    ShaderTokenStream &tokens, RHI::ShaderStageFlags groupDefaultStages) const
{
    using namespace ShaderCompilerDetail;

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
    if(bindingType != RHI::BindingType::Sampler && bindingType != RHI::BindingType::AccelerationStructure)
    {
        tokens.ConsumeOrThrow("<");
        while(tokens.GetCurrentToken() != ">")
        {
            if(tokens.IsFinished())
            {
                tokens.Throw("'>' expected");
            }
            templateParam += tokens.GetCurrentToken();
            tokens.Next();
        }
        tokens.ConsumeOrThrow(">");
    }

    std::optional<uint32_t> arraySize;
    bool isVariableArraySize = Category == BindingCategory::BindlessWithVariableSize;
    if(tokens.GetCurrentToken() == "[")
    {
        tokens.Next();
        if constexpr(Category == BindingCategory::Bindless || Category == BindingCategory::BindlessWithVariableSize)
        {
            try
            {
                arraySize = std::stoi(tokens.GetCurrentToken());
                tokens.Next();
            }
            catch(...)
            {
                tokens.Throw(fmt::format("Invalid array size: {}", tokens.GetCurrentToken()));
            }
        }
        else
        {
            try
            {
                arraySize = std::stoi(tokens.GetCurrentToken());
                tokens.Next();
            }
            catch(...)
            {
                tokens.Throw(fmt::format("Invalid array size: {}", tokens.GetCurrentToken()));
            }
        }
        tokens.ConsumeOrThrow("]");
    }
    else if constexpr(Category == BindingCategory::Bindless || Category == BindingCategory::BindlessWithVariableSize)
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

    RHI::ShaderStageFlags stages = groupDefaultStages;
    if(tokens.GetCurrentToken() == ",")
    {
        tokens.Next();
        stages = ParseStages(tokens);
    }

    return ParsedBinding
    {
        .name              = std::move(bindingName),
        .type              = bindingType,
        .rawTypename       = std::move(rawTypename),
        .arraySize         = arraySize,
        .stages            = stages,
        .templateParam     = std::move(templateParam),
        .bindless          = Category == BindingCategory::Bindless ||
                             Category == BindingCategory::BindlessWithVariableSize,
        .variableArraySize = isVariableArraySize
    };
}

ShaderCompiler::ParsedBindingAlias ShaderCompiler::ParseBindingAlias(ShaderTokenStream &tokens) const
{
    ParsedBindingAlias alias;
    assert(tokens.GetCurrentToken() == "rtrc_alias");
    tokens.Next();
    tokens.ConsumeOrThrow("(");

    alias.rawTypename = tokens.GetCurrentToken();
    tokens.Next();

    auto it = ShaderCompilerDetail::RESOURCE_PROPERTIES.find(alias.rawTypename);
    if(it == ShaderCompilerDetail::RESOURCE_PROPERTIES.end())
    {
        tokens.Throw(fmt::format("Unknown resource type: {}", alias.rawTypename));
    }
    alias.type = it->second;

    if(alias.type != RHI::BindingType::Sampler && alias.type != RHI::BindingType::AccelerationStructure)
    {
        tokens.ConsumeOrThrow("<");
        tokens.Next();
        while(tokens.GetCurrentToken() != ">")
        {
            if(tokens.IsFinished())
            {
                tokens.Throw("'>' expected");
            }
            alias.templateParam += tokens.GetCurrentToken();
        }
        tokens.Next();
    }

    tokens.ConsumeOrThrow(",");
    alias.name = tokens.GetCurrentToken();
    if(!ShaderTokenStream::IsIdentifier(alias.name))
    {
        tokens.Throw("Binding name expected");
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
    return alias;
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
    Shader::PushConstantRange &range, uint32_t &nextOffset, Shader::Category category) const
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
        range.stages = ShaderCompilerDetail::ShaderCategoryToStages(category);
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

ShaderCompiler::Bindings ShaderCompiler::CollectBindings(const std::string &source, Shader::Category category) const
{
    using namespace ShaderCompilerDetail;

    constexpr std::string_view RESOURCE                          = "rtrc_define";
    constexpr std::string_view ALIAS                             = "rtrc_alias";
    constexpr std::string_view BINDLESS                          = "rtrc_bindless";
    constexpr std::string_view BINDLESS_WITH_VARIABLE_ARRAY_SIZE = "rtrc_bindless_varsize";
    constexpr std::string_view GROUP                             = "rtrc_group";
    constexpr std::string_view SAMPLER                           = "rtrc_sampler";
    constexpr std::string_view PUSH_CONSTANT                     = "_rtrc_push_constant_impl";

    std::set<std::string>                parsedBindingNames;
    std::map<std::string, ParsedBinding> ungroupedBindings;

    std::vector<ParsedBindingAlias> parsedBindingAliases;

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
        const size_t bindlessPos     = FindKeyword(source, BINDLESS, keywordBeginPos);
        const size_t aliasPos        = FindKeyword(source, ALIAS, keywordBeginPos);
        const size_t varSizePos      = FindKeyword(source, BINDLESS_WITH_VARIABLE_ARRAY_SIZE, keywordBeginPos);
        const size_t groupPos        = FindKeyword(source, GROUP, keywordBeginPos);
        const size_t samplerPos      = FindKeyword(source, SAMPLER, keywordBeginPos);
        const size_t pushConstantPos = FindKeyword(source, PUSH_CONSTANT, keywordBeginPos);
        const size_t minPos = (std::min)({ resourcePos, bindlessPos, aliasPos, groupPos, samplerPos, pushConstantPos });

        if(minPos == std::string::npos)
        {
            break;
        }

        //std::string namespaceChain = GetNamespaceChain(source, minPos);

        if(minPos == resourcePos || minPos == bindlessPos || minPos == varSizePos)
        {
            keywordBeginPos = minPos + (minPos == resourcePos ? RESOURCE.size() : BINDLESS.size());
            ShaderTokenStream tokens(source, keywordBeginPos);
            tokens.ConsumeOrThrow("(");
            ParsedBinding binding;
            if(minPos == resourcePos)
            {
                binding = ParseBinding<false, BindingCategory::Regular>(tokens, RHI::ShaderStage::All);
            }
            else if(minPos == bindlessPos)
            {
                binding = ParseBinding<false, BindingCategory::Bindless>(tokens, RHI::ShaderStage::All);
            }
            else
            {
                binding = ParseBinding<false, BindingCategory::BindlessWithVariableSize>(tokens, RHI::ShaderStage::All);
            }
            if(parsedBindingNames.contains(binding.name))
            {
                tokens.Throw(fmt::format("Binding {} is defined multiple times", binding.name));
            }
            //binding.namespaceChain = std::move(namespaceChain);
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
                pushConstantRanges.emplace_back(), pushConstantRangeOffset, category);
            continue;
        }

        if(minPos == aliasPos)
        {
            keywordBeginPos = aliasPos + ALIAS.size();
            ShaderTokenStream tokens(source, aliasPos);
            parsedBindingAliases.push_back(ParseBindingAlias(tokens));
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
        RHI::ShaderStageFlags groupDefaultStages = RHI::ShaderStage::All;
        if(tokens.GetCurrentToken() == ",")
        {
            tokens.Next();
            groupDefaultStages = ParseStages(tokens);
        }
        tokens.ConsumeOrThrow(")");
        tokens.ConsumeOrThrow("{");
        if(parsedGroupNames.contains(groupName))
        {
            tokens.Throw(fmt::format("Group {} is already defined", groupName));
        }
        parsedGroupNames.insert(groupName);

        ParsedBindingGroup &currentGroup = parsedGroups.emplace_back();
        currentGroup.name = std::move(groupName);
        currentGroup.defaultStages = groupDefaultStages;

        /*if(!namespaceChain.empty())
        {
            throw Exception(fmt::format("Binding group {} is not defined in global namespace", currentGroup.name));
        }*/

        while(true)
        {
            if(tokens.GetCurrentToken() == RESOURCE ||
               tokens.GetCurrentToken() == BINDLESS ||
               tokens.GetCurrentToken() == BINDLESS_WITH_VARIABLE_ARRAY_SIZE)
            {
                const bool isRegular = tokens.GetCurrentToken() == RESOURCE;
                const bool isBindless = !isRegular && tokens.GetCurrentToken() == BINDLESS;
                tokens.Next();
                tokens.ConsumeOrThrow("(");
                if(RESOURCE_PROPERTIES.contains(tokens.GetCurrentToken()))
                {
                    auto &binding = currentGroup.bindings.emplace_back();
                    if(isRegular)
                    {
                        binding = ParseBinding<true, BindingCategory::Regular>(tokens, groupDefaultStages);
                    }
                    else if(isBindless)
                    {
                        binding = ParseBinding<true, BindingCategory::Bindless>(tokens, groupDefaultStages);
                    }
                    else
                    {
                        binding = ParseBinding<true, BindingCategory::BindlessWithVariableSize>(tokens, groupDefaultStages);
                    }
                    currentGroup.isRef.push_back(false);
                    if(parsedBindingNames.contains(binding.name))
                    {
                        tokens.Throw(fmt::format("Resource {} is defined multiple times", binding.name));
                    }
                    parsedBindingNames.insert(binding.name);
                }
                else
                {
                    tokens.Throw(fmt::format("Unknown resource type: {}", tokens.GetCurrentToken()));
                }
                tokens.ConsumeOrThrow(")");
            }
            else if(tokens.GetCurrentToken() == ALIAS)
            {
                parsedBindingAliases.push_back(ParseBindingAlias(tokens));
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
                    currentGroup.bindings.back().stages = groupDefaultStages;
                }
                tokens.ConsumeOrThrow(")");
            }
            else if(tokens.GetCurrentToken() == "rtrc_uniform")
            {
                tokens.Next();
                tokens.ConsumeOrThrow("(");
                currentGroup.uniformPropertyDefinitions.emplace_back();
                currentGroup.uniformPropertyDefinitions.back().type = tokens.GetCurrentToken();
                tokens.Next();
                while(tokens.GetCurrentToken() == "::")
                {
                    tokens.Next();
                    currentGroup.uniformPropertyDefinitions.back().type += "::" + tokens.GetCurrentToken();
                    tokens.Next();
                }
                tokens.ConsumeOrThrow(",");
                if(!ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
                {
                    tokens.Throw("Uniform property name expected");
                }
                currentGroup.uniformPropertyDefinitions.back().name = tokens.GetCurrentToken();
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

    bindings.aliases = std::move(parsedBindingAliases);

    bindings.inlineSamplerDescs = std::move(inlineSamplerDescs);
    bindings.inlineSamplerNameToDescIndex = std::move(inlineSamplerNameToDescIndex);

    bindings.pushConstantRanges = std::move(pushConstantRanges);
    bindings.pushConstantRangeContents = std::move(pushConstantRangeContents);
    bindings.pushConstantRangeNames = std::move(pushConstantRangeNames);

    return bindings;
}

std::string ShaderCompiler::GetNamespaceChain(const std::string &source, size_t endPos)
{
    std::vector<std::string> namespaceNames;
    size_t currentPos = 0;
    while(currentPos < endPos)
    {
        const size_t nextNamespacePos = ShaderCompilerDetail::FindKeyword(source, "namespace", currentPos);
        const size_t nextLeftBracePos = source.find('{', currentPos);
        const size_t nextRightBracePos = source.find('}', currentPos);
        const size_t nextPos = (std::min)(nextNamespacePos, (std::min)(nextLeftBracePos, nextRightBracePos));
        if(nextPos == std::string::npos)
        {
            break;
        }

        if(nextPos == nextNamespacePos)
        {
            ShaderTokenStream tokens(source, nextNamespacePos);
            assert(tokens.GetCurrentToken() == "namespace");
            tokens.Next();
            if(!ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
            {
                tokens.Throw(fmt::format("Invalid namespace name: {}", tokens.GetCurrentToken()));
            }
            namespaceNames.push_back(tokens.GetCurrentToken());
            tokens.Next();
            if(tokens.GetCurrentToken() != "{")
            {
                tokens.Throw("'{' expected after namespace definition");
            }
            namespaceNames.push_back("["); // Use '[' to indicate that this is a namespace
            currentPos = tokens.GetCurrentPosition();
            continue;
        }

        if(nextPos == nextLeftBracePos)
        {
            currentPos = nextLeftBracePos + 1;
            namespaceNames.push_back("{");
            continue;
        }
        
        assert(nextPos == nextRightBracePos);
        currentPos = nextRightBracePos + 1;
        if(namespaceNames.empty() || (namespaceNames.back() != "{" && namespaceNames.back() != "["))
        {
            ShaderTokenStream tokens(source, nextPos);
            tokens.Throw("Cannot find matching '{' for '}'");
        }
        if(namespaceNames.back() == "[")
        {
            namespaceNames.pop_back();
        }
        namespaceNames.pop_back();
    }

    std::string ret;
    for(size_t i = 0; i < namespaceNames.size(); i += 2)
    {
        if(namespaceNames[i] != "{")
        {
            if(!ret.empty())
            {
                ret += "::";
            }
            ret += namespaceNames[i];
        }
    }

    return ret;
}

RTRC_END
