#include <Rtrc/Core/Enumerate.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Shader/Compiler.h>
#include <Rtrc/ShaderCommon/Reflection/D3D12Reflection.h>
#include <Rtrc/ShaderCommon/Reflection/SPIRVReflection.h>

RTRC_BEGIN

void ShaderCompiler::SetDevice(Ref<Device> device)
{
    device_ = device;
}

RC<Shader> ShaderCompiler::Compile(
    const ShaderCompileEnvironment &envir,
    const CompilableShader         &shader,
    bool                            debug) const
{
    assert(device_);
    const RHI::BackendType backend = device_->GetBackendType();

    std::vector<std::string> includeDirs = envir.includeDirs;
    if(!shader.sourceFilename.empty())
    {
        std::string dir = std::filesystem::absolute(shader.sourceFilename).lexically_normal().parent_path().string();
        includeDirs.push_back(std::move(dir));
    }

    const bool VS = !shader.vertexEntry.empty();
    const bool FS = !shader.fragmentEntry.empty();
    const bool CS = !shader.computeEntry.empty();
    const bool RT = shader.isRayTracingShader;

    ShaderInfo::Category category;
    if(VS && FS && !CS && !RT)
    {
        category = ShaderInfo::Category::Graphics;
    }
    else if(!VS && !FS && CS && !RT)
    {
        category = ShaderInfo::Category::Compute;
    }
    else if(!VS && !FS && !CS && RT)
    {
        category = ShaderInfo::Category::RayTracing;
    }
    else
    {
        throw Exception(fmt::format(
            "Invalid shader category. Shader name: {}; VS: {}, FS: {}, CS: {}, RT: {}",
            shader.name, VS, FS, CS, RT));
    }

    struct BindingSlot
    {
        int set;
        int index;
    };
    std::map<std::string, BindingSlot> bindingNameToSlot;
    std::vector<RC<BindingGroupLayout>> groupLayouts;

    // Create regular binding group layouts

    for(auto &&[groupIndex, group] : Enumerate(shader.bindingGroups))
    {
        BindingGroupLayout::Desc groupLayoutDesc;
        for(auto &&[bindingIndex, binding] : Enumerate(group.bindings))
        {
            BindingGroupLayout::BindingDesc &bindingDesc = groupLayoutDesc.bindings.emplace_back();
            bindingDesc.type      = binding.type;
            bindingDesc.stages    = binding.stages;
            bindingDesc.arraySize = binding.arraySize;
            bindingDesc.bindless  = binding.bindless;

            if(binding.variableArraySize)
            {
                if(&binding != &group.bindings.back())
                {
                    throw Exception(fmt::format(
                        "Binding {} in shader {} has variable array size but is not the last one in binding group {}",
                        binding.name, shader.name, group.name));
                }
                groupLayoutDesc.variableArraySize = true;
            }
            
            bindingNameToSlot[binding.name] = BindingSlot
            {
                .set = static_cast<int>(groupIndex),
                .index = static_cast<int>(bindingIndex)
            };
        }

        if(!group.uniformPropertyDefinitions.empty())
        {
            groupLayoutDesc.bindings.push_back(
            {
                .type = RHI::BindingType::ConstantBuffer,
                .stages = group.stages
            });
            bindingNameToSlot[group.name] = BindingSlot
            {
                .set = static_cast<int>(groupIndex),
                .index = static_cast<int>(group.bindings.size())
            };
        }

        // Binding with variable array size must be the last one in group
        // In that case, swap the slots of variable-sized binding and uniform-variable binding
        if(!group.uniformPropertyDefinitions.empty() && !group.bindings.empty() && groupLayoutDesc.variableArraySize)
        {
            const size_t a = groupLayoutDesc.bindings.size() - 2;
            const size_t b = a + 1;
            std::swap(groupLayoutDesc.bindings[a], groupLayoutDesc.bindings[b]);
            std::swap(bindingNameToSlot[group.name], bindingNameToSlot[group.bindings.back().name]);
        }

        groupLayouts.push_back(device_->CreateBindingGroupLayout(groupLayoutDesc));
    }

    // Group for inline samplers

    int inlineSamplerGroupIndex = -1;
    if(!shader.inlineSamplerDescs.empty())
    {
        std::vector<RC<Sampler>> samplers;
        for(auto &s : shader.inlineSamplerDescs)
        {
            samplers.push_back(device_->CreateSampler(s));
        }
        BindingGroupLayout::Desc groupLayoutDesc;
        groupLayoutDesc.bindings.push_back(BindingGroupLayout::BindingDesc
        {
            .type = RHI::BindingType::Sampler,
            .stages = GetFullShaderStages(category),
            .arraySize = static_cast<uint32_t>(shader.inlineSamplerDescs.size()),
            .immutableSamplers = std::move(samplers)
        });
        inlineSamplerGroupIndex = static_cast<int>(groupLayouts.size());
        bindingNameToSlot["_rtrcGeneratedSamplers"] = BindingSlot{ .set = inlineSamplerGroupIndex, .index = 0 };
        groupLayouts.push_back(device_->CreateBindingGroupLayout(groupLayoutDesc));
    }

    // Macros

    std::map<std::string, std::string> macros = envir.macros;

    // Keywords

    for(size_t i = 0; i < shader.keywords.size(); ++i)
    {
        const int value = shader.keywordValues[i].value;
        shader.keywords[i].Match(
        [&](const BoolShaderKeyword &keyword)
        {
            macros.insert({ keyword.name, value ? "1" : "0" });
        },
        [&](const EnumShaderKeyword &keyword)
        {
            macros.insert({ keyword.name, std::to_string(value) });
            for(auto &&[valueIndex, valueName] : Enumerate(keyword.values))
            {
                macros.insert(
                {
                    fmt::format("{}_{}", keyword.name, valueName),
                    std::to_string(valueIndex)
                });
            }
        });
    }

    // rtrc_* macros

    macros["rtrc_symbol_name(...)"] = "";
    macros["rtrc_vertex(...)"]      = "";
    macros["rtrc_vert(...)"]        = "";
    macros["rtrc_pixel(...)"]       = "";
    macros["rtrc_fragment(...)"]    = "";
    macros["rtrc_frag(...)"]        = "";
    macros["rtrc_compute(...)"]     = "";
    macros["rtrc_comp(...)"]        = "";
    macros["rtrc_raytracing(...)"]  = "";
    macros["rtrc_rt(...)"]          = "";
    macros["rtrc_rt_group(...)"]    = "";
    macros["rtrc_keyword(...)"]     = "";

    macros["rtrc_group(NAME, ...)"]                  = "_rtrc_group_##NAME";
    macros["rtrc_define(TYPE, NAME, ...)"]           = "_rtrc_define_##NAME";
    macros["rtrc_alias(TYPE, NAME, ...)"]            = "_rtrc_alias_##NAME";
    macros["rtrc_bindless(TYPE, NAME, ...)"]         = "_rtrc_define_##NAME";
    macros["rtrc_bindless_varsize(TYPE, NAME, ...)"] = "_rtrc_define_##NAME";
    macros["rtrc_uniform(TYPE, NAME)"]               = "";
    macros["rtrc_sampler(NAME, ...)"]                = "_rtrc_sampler_##NAME";
    macros["rtrc_ref(NAME, ...)"]                    = "";
    macros["rtrc_group_struct(NAME, ...)"]           = "_rtrc_group_struct_##NAME";
    macros["rtrc_push_constant(NAME, ...)"]          = "_rtrc_push_constant_##NAME";

    macros["_rtrc_generated_shader_prefix"]; // Make sure _rtrc_generated_shader_prefix is defined
    
    bool hasBindless = false;
    std::vector<ShaderUniformBlock> uniformBlocks;
    std::map<std::string, const ParsedBinding *> nameToBinding;
    std::map<std::string, std::string> bindingNameToRegisterSpecifier; // For D3D12 backend

    // Macros for resources and binding groups

    for(auto &group : shader.bindingGroups)
    {
        int t = 0, b = 0, s = 0, u = 0;
        std::string groupLeft = fmt::format("_rtrc_group_{}", group.name);
        std::string groupDefinition;

        for(auto &&[bindingIndex, binding] : Enumerate(group.bindings))
        {
            const auto [set, slot] = bindingNameToSlot.at(binding.name);
            nameToBinding[binding.name] = &binding;

            std::string arraySpecifier = GetArraySpecifier(binding.arraySize);

            std::string templateParamSpecifier;
            if(!binding.templateParam.empty())
            {
                templateParamSpecifier = fmt::format("<{}>", binding.templateParam);
            }

            std::string left = fmt::format("_rtrc_define_{}", binding.name);
            std::string right;
            if(backend == RHI::BackendType::Vulkan)
            {
                right = fmt::format(
                    "[[vk::binding({}, {})]] {}{} {}{};",
                    slot, set, binding.rawTypeName, templateParamSpecifier,
                    binding.name, arraySpecifier);
            }
            else
            {
                assert(backend == RHI::BackendType::DirectX12);
                std::string reg;
                switch(BindingTypeToD3D12RegisterType(binding.type))
                {
                case D3D12RegisterType::S: reg = "s" + std::to_string(s++); break;
                case D3D12RegisterType::B: reg = "b" + std::to_string(b++); break;
                case D3D12RegisterType::T: reg = "t" + std::to_string(t++); break;
                case D3D12RegisterType::U: reg = "u" + std::to_string(u++); break;
                }
                std::string regSpec = fmt::format("{}, space{}", reg, set);
                bindingNameToRegisterSpecifier.insert({ binding.name, regSpec });
                right = fmt::format(
                    "{}{} {}{} : register({});",
                    binding.rawTypeName, templateParamSpecifier, binding.name, arraySpecifier, regSpec);
            }

            if(group.isRef[bindingIndex])
            {
                // Referenced resources are defined outside of the group.
                // Use the original rtrc_define(...) to generate the final definition.
                macros.insert({ std::move(left), std::move(right) });
            }
            else
            {
                // Local rtrc_define(...) can be safely removed.
                // The actual resource definition is moved to group beginning
                macros.insert({ std::move(left), "" });
                groupDefinition += right;
            }

            hasBindless |= binding.bindless;
        }

        if(!group.uniformPropertyDefinitions.empty())
        {
            const auto [set, slot] = bindingNameToSlot.at(group.name);

            auto &uniformBlock = uniformBlocks.emplace_back();
            uniformBlock.name         = group.name;
            uniformBlock.group        = set;
            uniformBlock.indexInGroup = slot;

            std::string uniformPropertyStr;
            uniformBlock.variables.reserve(group.uniformPropertyDefinitions.size());
            for(const ParsedUniformDefinition &uniform : group.uniformPropertyDefinitions)
            {
                uniformPropertyStr += fmt::format("{} {};", uniform.type, uniform.name);

                auto &var = uniformBlock.variables.emplace_back();
                var.name       = uniform.name;
                var.pooledName = ShaderPropertyName(var.name);
                var.type       = ShaderUniformBlock::GetTypeFromTypeName(uniform.type);
            }

            if(backend == RHI::BackendType::Vulkan)
            {
                groupDefinition += fmt::format(
                    "struct _rtrc_generated_cbuffer_struct_{} {{ {} }}; "
                    "[[vk::binding({}, {})]] ConstantBuffer<_rtrc_generated_cbuffer_struct_{}> {};",
                    group.name, uniformPropertyStr,
                    slot, set, group.name, group.name);
            }
            else
            {
                assert(backend == RHI::BackendType::DirectX12);
                groupDefinition += fmt::format(
                    "struct _rtrc_generated_cbuffer_struct_{} {{ {} }}; "
                    "ConstantBuffer<_rtrc_generated_cbuffer_struct_{}> {} : register(b{}, space{});",
                    group.name, uniformPropertyStr,
                    group.name, group.name, b++, set);
            }
        }
        
        std::string groupRight = fmt::format("struct _group_dummy_struct_{}", group.name);
        macros.insert({ groupLeft, std::move(groupRight) });

        macros[groupLeft] = groupDefinition + macros[groupLeft];
    }

    // Macros for push constant ranges

    if(backend == RHI::BackendType::Vulkan)
    {
        std::string pushConstantContent;
        for(auto &range : shader.pushConstantRanges)
        {
            for(auto &var : range.variables)
            {
                pushConstantContent += fmt::format(
                    "[[vk::offset({})]] {} {}__{};",
                    var.offset, ParsedPushConstantVariable::TypeToName(var.type), range.name, var.name);
            }
        }
        for(size_t i = 0; i < shader.pushConstantRanges.size(); ++i)
        {
            const std::string &name = shader.pushConstantRanges[i].name;
            std::string left = fmt::format("_rtrc_push_constant_{}", name);
            std::string right;
            if(i == 0)
            {
                right = fmt::format(
                    "struct _rtrc_push_constant_struct {{ {} }}; "
                    "[[vk::push_constant]] _rtrc_push_constant_struct _rtrc_push_constant_struct_buffer;",
                    pushConstantContent);
            }
            right += fmt::format("struct _rtrc_push_constant_dummy_struct_{}", name);
            macros.insert({ std::move(left), std::move(right) });
        }
        macros.insert(
        {
            "rtrc_get_push_constant(RANGE, VAR)",
            "(_rtrc_push_constant_struct_buffer.RANGE##__##VAR)"
        });
    }
    else
    {
        assert(backend == RHI::BackendType::DirectX12);
        const size_t pushConstantRegisterSpace = groupLayouts.size();
        for(auto &&[rangeIndex, range] : Enumerate(shader.pushConstantRanges))
        {
            std::string content;
            size_t offset = 0, padIndex = 0;
            for(auto &var : range.variables)
            {
                const size_t size = ParsedPushConstantVariable::TypeToSize(var.type);
                const size_t next16ByteOffset = UpAlignTo<size_t>(offset + 1, 16);
                size_t alignedOffset = offset;
                if(offset + size > next16ByteOffset)
                {
                    alignedOffset = next16ByteOffset;
                }
                for(size_t i = offset; i < alignedOffset; ++i)
                {
                    content += fmt::format("float pad{};", padIndex++);
                }
                content += fmt::format("{} {};", ParsedPushConstantVariable::TypeToName(var.type), var.name);
                offset = alignedOffset + size;
            }

            std::string left = fmt::format("_rtrc_push_constant_{}", range.name);
            std::string right = fmt::format(
                "struct _rtrc_push_constant_struct_{} {{ {} }}; "
                "ConstantBuffer<_rtrc_push_constant_struct_{}> _rtrc_push_constant_range_{} : register(b{}, space{});"
                "struct _rtrc_push_constant_dummy_struct_{}",
                range.name, content, range.name, range.name,
                rangeIndex, pushConstantRegisterSpace, rangeIndex);
            macros.insert({ std::move(left), std::move(right) });
        }
        macros.insert(
        {
            "rtrc_get_push_constant(RANGE, VAR)",
            "(_rtrc_push_constant_range_##RANGE.VAR)"
        });
    }

    // Macros for inline samplers

    if(inlineSamplerGroupIndex >= 0)
    {
        if(backend == RHI::BackendType::Vulkan)
        {
            macros["_rtrc_generated_shader_prefix"] += fmt::format(
                "[[vk::binding(0, {})]] SamplerState _rtrc_generated_samplers[{}];",
                inlineSamplerGroupIndex, shader.inlineSamplerDescs.size());

            for(auto &[name, index] : shader.inlineSamplerNameToDesc)
            {
                std::string left = fmt::format("_rtrc_sampler_{}", name);
                std::string right = fmt::format("static SamplerState {} = _rtrc_generated_samplers[{}];", name, index);
                macros.insert({ std::move(left), std::move(right) });
            }
        }
        else
        {
            assert(backend == RHI::BackendType::DirectX12);
            std::string generatedShaderPrefix;
            for(auto &&[i, s] : Enumerate(shader.inlineSamplerDescs))
            {
                generatedShaderPrefix += fmt::format(
                    "SamplerState _rtrc_generated_sampler_{} : register(s{}, space{});",
                    i, i, inlineSamplerGroupIndex);
            }
            macros["_rtrc_generated_shader_prefix"] += generatedShaderPrefix;

            for(auto &&[name, index] : shader.inlineSamplerNameToDesc)
            {
                std::string left = fmt::format("_rtrc_sampler_{}", name);
                std::string right = fmt::format("static SamplerState {} = _rtrc_generated_sampler_{};", name, index);
                macros.insert({ std::move(left), std::move(right) });
            }
        }
    }

    // Macros for ungrouped bindings

    const int groupIndexForUngroupedBindings = groupLayouts.size(); // Only for vulkan backend
    for(auto &&[bindingIndex, binding] : Enumerate(shader.ungroupedBindings))
    {
        nameToBinding.insert({ binding.name, &binding });

        std::string templateParamSpec;
        if(!binding.templateParam.empty())
        {
            templateParamSpec = fmt::format("<{}>", binding.templateParam);
        }
        std::string arraySpec = GetArraySpecifier(binding.arraySize);

        std::string left = fmt::format("_rtrc_define_{}", binding.name);
        std::string right;
        if(backend == RHI::BackendType::Vulkan)
        {
            right = fmt::format(
                "[[vk::binding({}, {})]] {}{} {}{};",
                bindingIndex, groupIndexForUngroupedBindings,
                binding.rawTypeName, templateParamSpec, binding.name, arraySpec);
            bindingNameToSlot.insert(
            {
                binding.name,
                BindingSlot{ groupIndexForUngroupedBindings, static_cast<int>(bindingIndex) }
            });
        }
        else
        {
            assert(backend == RHI::BackendType::DirectX12);
            const char *reg;
            switch(BindingTypeToD3D12RegisterType(binding.type))
            {
            case D3D12RegisterType::B: reg = "b"; break;
            case D3D12RegisterType::T: reg = "t"; break;
            case D3D12RegisterType::S: reg = "s"; break;
            case D3D12RegisterType::U: reg = "u"; break;
            default: Unreachable();
            }
            std::string regSpec = fmt::format("{}0, space{}", reg, 9000 + bindingIndex);
            bindingNameToRegisterSpecifier.insert({ binding.name, regSpec });
            right = fmt::format(
                "{}{} {}{} : register({});",
                binding.rawTypeName, templateParamSpec, binding.name, arraySpec, regSpec);
            bindingNameToSlot.insert(
            {
                binding.name,
                BindingSlot{ 9000 + static_cast<int>(bindingIndex), 0 }
            });
        }
        macros.insert({ std::move(left), std::move(right) });
    }

    // Macros for aliased bindings

    for(auto &&[aliasIndex, alias] : Enumerate(shader.aliases))
    {
        int set, slot;
        if(auto it = bindingNameToSlot.find(alias.aliasedName); it != bindingNameToSlot.end())
        {
            set = it->second.set;
            slot = it->second.index;
        }
        else
        {
            throw Exception(fmt::format("Unknown aliased name {}", alias.aliasedName));
        }
        auto &binding = *nameToBinding.at(alias.aliasedName);

        std::string templateParamSpec;
        if(!alias.templateParam.empty())
        {
            templateParamSpec = fmt::format("<{}>", alias.templateParam);
        }

        std::string left = fmt::format("_rtrc_alias_{}", alias.name);
        std::string right;
        if(backend == RHI::BackendType::Vulkan)
        {
            right = fmt::format(
                "[[vk::binding({}, {})]] {}{} {}{};",
                slot, set, alias.rawTypename, templateParamSpec,
                alias.name, GetArraySpecifier(binding.arraySize));
        }
        else
        {
            assert(backend == RHI::BackendType::DirectX12);
            const char *reg;
            switch(BindingTypeToD3D12RegisterType(alias.bindingType))
            {
            case D3D12RegisterType::B: reg = "b"; break;
            case D3D12RegisterType::T: reg = "t"; break;
            case D3D12RegisterType::S: reg = "s"; break;
            case D3D12RegisterType::U: reg = "u"; break;
            default: Unreachable();
            }
            right = fmt::format(
                "{}{} {}{} : register({}0, space{});",
                alias.rawTypename, templateParamSpec, alias.name,
                GetArraySpecifier(binding.arraySize),
                reg, 1000 + aliasIndex);
        }
        macros.insert({ std::move(left), std::move(right) });
    }

    // Compile

    DXC::ShaderInfo shaderInfo;
    shaderInfo.source         = shader.source;
    shaderInfo.sourceFilename = shader.sourceFilename;
    shaderInfo.macros         = macros;
    shaderInfo.includeDirs    = std::move(includeDirs);
    shaderInfo.bindless       = hasBindless;

    if(RT)
    {
        shaderInfo.rayTracing = true;
        shaderInfo.rayQuery = true;
    }
    else
    {
        auto HasASBinding = [&]
        {
            for(auto &group : shader.bindingGroups)
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

    std::vector<unsigned char> vsData, fsData, csData, rtData;
    Box<ShaderReflection> vsRefl, fsRefl, csRefl, rtRefl;
    if(VS)
    {
        shaderInfo.entryPoint = shader.vertexEntry;
        DoCompilation(shaderInfo, CompileStage::VS, debug, vsData, vsRefl);
    }
    if(FS)
    {
        shaderInfo.entryPoint = shader.fragmentEntry;
        DoCompilation(shaderInfo, CompileStage::FS, debug, fsData, fsRefl);
    }
    if(CS)
    {
        shaderInfo.entryPoint = shader.computeEntry;
        DoCompilation(shaderInfo, CompileStage::CS, debug, csData, csRefl);
    }
    if(RT)
    {
        DoCompilation(shaderInfo, CompileStage::RT, debug, rtData, rtRefl);
    }

    // Check uses of ungrouped bindings
    
    EnsureAllUsedBindingsAreGrouped(shader, vsRefl, "vertex shader");
    EnsureAllUsedBindingsAreGrouped(shader, fsRefl, "fragment shader");
    EnsureAllUsedBindingsAreGrouped(shader, csRefl, "compute shader");
    EnsureAllUsedBindingsAreGrouped(shader, rtRefl, "ray tracing shader");

    // Fill shader object

    auto ret = MakeRC<Shader>();
    auto info = MakeRC<ShaderInfo>();
    ret->info_ = info;

    info->shaderBindingLayoutInfo_ = MakeRC<ShaderBindingLayoutInfo>();
    info->category_ = category;

    if(VS)
    {
        ret->rawShaders_[Shader::VS_INDEX] = device_->GetRawDevice()->CreateShader(
            vsData.data(), vsData.size(), { { RHI::ShaderStage::VertexShader, shader.vertexEntry } });
        info->VSInput_ = vsRefl->GetInputVariables();
    }
    if(FS)
    {
        ret->rawShaders_[Shader::FS_INDEX] = device_->GetRawDevice()->CreateShader(
            fsData.data(), fsData.size(), { { RHI::ShaderStage::FragmentShader, shader.fragmentEntry } });
    }
    if(CS)
    {
        ret->rawShaders_[Shader::CS_INDEX] = device_->GetRawDevice()->CreateShader(
            csData.data(), csData.size(), { { RHI::ShaderStage::ComputeShader, shader.computeEntry } });
    }
    if(RT)
    {
        std::vector<RHI::RawShaderEntry> rtEntries = rtRefl->GetEntries();
        auto FindEntryIndex = [&rtEntries](std::string_view name)
        {
            for(size_t i = 0; i < rtEntries.size(); ++i)
            {
                if(rtEntries[i].name == name)
                {
                    return i;
                }
            }
            throw Exception(fmt::format("Ray tracing entry {} is required by a shader group but not found", name));
        };

        auto shaderGroups = MakeRC<ShaderGroupInfo>();
        for(const std::vector<std::string> &rawGroup : shader.entryGroups)
        {
            assert(!rawGroup.empty());
            const uint32_t firstEntryIndex = FindEntryIndex(rawGroup[0]);
            if(rtEntries[firstEntryIndex].stage == RHI::ShaderStage::RT_RayGenShader)
            {
                if(rawGroup.size() != 1)
                {
                    throw Exception("RayGen shader group must contains no more than one shader");
                }
                shaderGroups->rayGenShaderGroups_.push_back({ firstEntryIndex });
            }
            else if(rtEntries[firstEntryIndex].stage == RHI::ShaderStage::RT_MissShader)
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
                    const RHI::ShaderStage stage = rtEntries[index].stage;
                    if(stage == RHI::ShaderStage::RT_ClosestHitShader)
                    {
                        if(group.closestHitShaderIndex != RHI::RAY_TRACING_UNUSED_SHADER)
                        {
                            throw Exception("Closest hit entry is already specified");
                        }
                        group.closestHitShaderIndex = index;
                    }
                    else if(stage == RHI::ShaderStage::RT_AnyHitShader)
                    {
                        if(group.anyHitShaderIndex != RHI::RAY_TRACING_UNUSED_SHADER)
                        {
                            throw Exception("Any hit entry is already specified");
                        }
                        group.anyHitShaderIndex = index;
                    }
                    else if(stage == RHI::ShaderStage::RT_IntersectionShader)
                    {
                        if(group.intersectionShaderIndex != RHI::RAY_TRACING_UNUSED_SHADER)
                        {
                            throw Exception("Intersection entry is already specified");
                        }
                        group.intersectionShaderIndex = index;
                    }
                    else
                    {
                        throw Exception(fmt::format(
                            "Unsupported shader stage in ray tracing shader group: {}",
                            std::to_underlying(stage)));
                    }
                };

                HandleEntryIndex(firstEntryIndex);
                for(size_t i = 1; i < rawGroup.size(); ++i)
                {
                    const uint32_t entryIndex = FindEntryIndex(rawGroup[i]);
                    HandleEntryIndex(entryIndex);
                }

                if(group.closestHitShaderIndex == RHI::RAY_TRACING_UNUSED_SHADER)
                {
                    throw Exception("A shader group must contain one of raygen/miss/closethit shader");
                }
                shaderGroups->hitShaderGroups_.push_back(group);
            }
        }

        info->shaderGroupInfo_ = std::move(shaderGroups);
        ret->rawShaders_[Shader::RT_INDEX] = device_->GetRawDevice()->CreateShader(
            rtData.data(), rtData.size(), std::move(rtEntries));
    }

    auto &bindingNameMap = info->shaderBindingLayoutInfo_->bindingNameMap_;
    bindingNameMap.allBindingNames_.resize(groupLayouts.size());
    for(size_t i = 0; i < groupLayouts.size(); ++i)
    {
        const size_t bindingCount = groupLayouts[i]->GetRHIObject()->GetDesc().bindings.size();
        bindingNameMap.allBindingNames_[i].resize(bindingCount);
    }
    for(auto &[name, slot] : bindingNameToSlot)
    {
        bindingNameMap.nameToSetAndSlot_[name] = { slot.set, slot.index };
        bindingNameMap.allBindingNames_[slot.set][slot.index] = name;
    }

    bindingNameMap.allPooledBindingNames_.resize(bindingNameMap.allBindingNames_.size());
    for(size_t i = 0; i < bindingNameMap.allBindingNames_.size(); ++i)
    {
        std::transform(
            bindingNameMap.allBindingNames_[i].begin(),
            bindingNameMap.allBindingNames_[i].end(),
            std::back_inserter(bindingNameMap.allPooledBindingNames_[i]),
            [](const std::string &name) { return GeneralPooledString(name); });
    }

    std::map<std::string, int, std::less<>> nameToGroupIndex;
    std::vector<std::string> groupNames;
    groupNames.resize(shader.bindingGroups.size());
    for(auto &&[index, group] : Enumerate(shader.bindingGroups))
    {
        nameToGroupIndex[group.name] = index;
        groupNames[index] = group.name;
    }
    info->shaderBindingLayoutInfo_->nameToBindingGroupLayoutIndex_ = std::move(nameToGroupIndex);
    info->shaderBindingLayoutInfo_->bindingGroupLayouts_           = std::move(groupLayouts);
    info->shaderBindingLayoutInfo_->bindingGroupNames_             = std::move(groupNames);

    BindingLayout::Desc bindingLayoutDesc;
    for(auto &group : info->shaderBindingLayoutInfo_->bindingGroupLayouts_)
    {
        bindingLayoutDesc.groupLayouts.push_back(group);
    }
    std::ranges::transform(
        shader.pushConstantRanges, std::back_inserter(bindingLayoutDesc.pushConstantRanges),
        [](const ParsedPushConstantRange &range) { return range.range; });

    if(backend == RHI::BackendType::DirectX12)
    {
        for(auto &&[aliasIndex, alias] : Enumerate(shader.aliases))
        {
            bindingLayoutDesc.unboundedAliases.push_back(RHI::UnboundedBindingArrayAliasing
            {
                .srcGroup = bindingNameToSlot.at(alias.aliasedName).set
            });
        }
    }
    info->shaderBindingLayoutInfo_->bindingLayout_ = device_->CreateBindingLayout(bindingLayoutDesc);

    if(inlineSamplerGroupIndex >= 0)
    {
        info->shaderBindingLayoutInfo_->bindingGroupForInlineSamplers_ =
            info->shaderBindingLayoutInfo_->bindingGroupLayouts_.back()->CreateBindingGroup();
    }
    
    {
        using enum Shader::BuiltinBindingGroup;
        auto SetBuiltinBindingGroupIndex = [&](Shader::BuiltinBindingGroup group, std::string_view name)
        {
            const int index = ret->GetBindingGroupIndexByName(name);
            info->shaderBindingLayoutInfo_->builtinBindingGroupIndices_[std::to_underlying(group)] = index;
        };
        SetBuiltinBindingGroupIndex(Pass,                   "Pass");
        SetBuiltinBindingGroupIndex(Material,               "Material");
        SetBuiltinBindingGroupIndex(Object,                 "Object");
        SetBuiltinBindingGroupIndex(BindlessTexture,        "GlobalBindlessTextureGroup");
        SetBuiltinBindingGroupIndex(BindlessGeometryBuffer, "GlobalBindlessGeometryBufferGroup");
    }

    if(CS)
    {
        info->computeShaderThreadGroupSize_ = csRefl->GetComputeShaderThreadGroupSize();
        ret->computePipeline_ = device_->CreateComputePipeline(ret);
    }

    info->shaderBindingLayoutInfo_->pushConstantRanges_ = std::move(bindingLayoutDesc.pushConstantRanges);
    info->shaderBindingLayoutInfo_->uniformBlocks_ = std::move(uniformBlocks);

    return ret;
}

RHI::ShaderStageFlags ShaderCompiler::GetFullShaderStages(ShaderInfo::Category category)
{
    switch(category)
    {
    case ShaderInfo::Category::Graphics:   return RHI::ShaderStage::AllGraphics;
    case ShaderInfo::Category::Compute:    return RHI::ShaderStage::CS;
    case ShaderInfo::Category::RayTracing: return RHI::ShaderStage::AllRT;
    default: Unreachable();
    }
}

ShaderCompiler::D3D12RegisterType ShaderCompiler::BindingTypeToD3D12RegisterType(RHI::BindingType type)
{
    using enum RHI::BindingType;
    switch(type)
    {
    case Texture:
    case Buffer:
    case StructuredBuffer:
    case AccelerationStructure:
        return D3D12RegisterType::T;
    case ConstantBuffer:
        return D3D12RegisterType::B;
    case Sampler:
        return D3D12RegisterType::S;
    case RWTexture:
    case RWBuffer:
    case RWStructuredBuffer:
        return D3D12RegisterType::U;
    default:
        Unreachable();
    }
}

std::string ShaderCompiler::GetArraySpecifier(std::optional<uint32_t> arraySize)
{
    return arraySize ? fmt::format("[{}]", *arraySize) : std::string();
}

void ShaderCompiler::DoCompilation(
    const DXC::ShaderInfo      &shaderInfo,
    CompileStage                stage,
    bool                        debug,
    std::vector<unsigned char> &outData,
    Box<ShaderReflection>      &outRefl) const
{
    DXC::Target target;
    const bool isVulkan = device_->GetBackendType() == RHI::BackendType::Vulkan;
    switch(stage)
    {
    case CompileStage::VS: target = isVulkan ? DXC::Target::Vulkan_1_3_VS_6_6 : DXC::Target::DirectX12_VS_6_6; break;
    case CompileStage::FS: target = isVulkan ? DXC::Target::Vulkan_1_3_FS_6_6 : DXC::Target::DirectX12_FS_6_6; break;
    case CompileStage::CS: target = isVulkan ? DXC::Target::Vulkan_1_3_CS_6_6 : DXC::Target::DirectX12_CS_6_6; break;
    case CompileStage::RT: target = isVulkan ? DXC::Target::Vulkan_1_3_RT_6_6 : DXC::Target::DirectX12_RT_6_6; break;
    default: Unreachable();
    }

    if(isVulkan)
    {
        outData = dxc_.Compile(shaderInfo, target, debug, nullptr, nullptr);
        outRefl = MakeBox<SPIRVReflection>(outData, shaderInfo.entryPoint);
    }
    else
    {
        assert(device_->GetBackendType() == RHI::BackendType::DirectX12);
        std::vector<std::byte> reflData;
        outData = dxc_.Compile(shaderInfo, target, debug, nullptr, &reflData);
        outRefl = MakeBox<D3D12Reflection>(dxc_.GetDxcUtils(), reflData, target == DXC::Target::DirectX12_RT_6_6);
    }
}

void ShaderCompiler::EnsureAllUsedBindingsAreGrouped(
    const CompilableShader &shader,
    const Box<ShaderReflection> &refl,
    std::string_view             stage) const
{
    if(refl)
    {
        for(auto &binding : shader.ungroupedBindings)
        {
            if(refl->IsBindingUsed(binding.name))
            {
                throw Exception(fmt::format(
                    "Binding {} is not explicitly grouped but is used in {}", binding.name, stage));
            }
        }
    }
}

RTRC_END
