#include <Rtrc/Core/Enumerate.h>
#include <Rtrc/Core/String.h>
#include <Rtrc/ShaderCommon/Preprocess/ShaderPreprocessing.h>


RTRC_BEGIN

namespace ShaderPreprocessingDetail
{

    enum class D3D12RegisterType
    {
        B, // Constant buffer
        S, // Sampler
        T, // SRV
        U, // UAV
    };

    RHI::ShaderStageFlags GetFullShaderStages(ShaderCategory category)
    {
        switch(category)
        {
        case ShaderCategory::ClassicalGraphics:   return RHI::ShaderStage::AllClassical;
        case ShaderCategory::MeshGraphics:        return RHI::ShaderStage::AllMesh;
        case ShaderCategory::Compute:             return RHI::ShaderStage::CS;
        case ShaderCategory::RayTracing:          return RHI::ShaderStage::AllRT;
        }
        Unreachable();
    }

    std::string GetArraySpecifier(std::optional<uint32_t> arraySize)
    {
        return arraySize ? fmt::format("[{}]", *arraySize) : std::string();
    }

    D3D12RegisterType BindingTypeToD3D12RegisterType(RHI::BindingType type)
    {
        using enum RHI::BindingType;
        using enum D3D12RegisterType;
        switch(type)
        {
        case Texture:
        case Buffer:
        case StructuredBuffer:
        case ByteAddressBuffer:
        case AccelerationStructure:
            return T;
        case ConstantBuffer:
            return B;
        case Sampler:
            return S;
        case RWTexture:
        case RWBuffer:
        case RWStructuredBuffer:
        case RWByteAddressBuffer:
            return U;
        }
        Unreachable();
    }

} // namespace ShaderPreprocessingDetail

ShaderPreprocessingOutput PreprocessShader(const ShaderPreprocessingInput &input, RHI::BackendType backend)
{
    std::vector<std::string> includeDirs = input.envir.includeDirs;
    if(!input.sourceFilename.empty())
    {
        std::string dir = std::filesystem::absolute(input.sourceFilename).lexically_normal().parent_path().string();
        includeDirs.push_back(std::move(dir));
    }

    const bool VS = !input.vertexEntry.empty();
    const bool FS = !input.fragmentEntry.empty();
    const bool CS = !input.computeEntry.empty();
    const bool TS = !input.taskEntry.empty();
    const bool MS = !input.meshEntry.empty();
    const bool RT = input.isRayTracingShader;

    ShaderCategory category;
    if(VS && FS && !CS && !RT && !TS && !MS)
    {
        category = ShaderCategory::ClassicalGraphics;
    }
    else if(!VS && !FS && CS && !RT && !TS && !MS)
    {
        category = ShaderCategory::Compute;
    }
    else if(!VS && !FS && !CS && RT && !TS && !MS)
    {
        category = ShaderCategory::RayTracing;
    }
    else if(!VS && FS && !CS && !RT && MS)
    {
        category = ShaderCategory::MeshGraphics;
    }
    else
    {
        throw Exception(fmt::format(
            "Invalid shader category. Shader name: {}; VS: {}, FS: {}, CS: {}, RT: {}, TS: {}, MS: {}",
            input.name, VS, FS, CS, RT, TS, MS));
    }

    struct BindingSlot
    {
        int set;
        int index;
    };
    std::map<std::string, BindingSlot> bindingNameToSlot;
    std::vector<RHI::BindingGroupLayoutDesc> groupLayoutDescs;
    
    // Create regular binding group layouts

    for(auto &&[groupIndex, group] : Enumerate(input.bindingGroups))
    {
        RHI::BindingGroupLayoutDesc groupLayoutDesc;
        for(auto &&[bindingIndex, binding] : Enumerate(group.bindings))
        {
            RHI::BindingDesc &bindingDesc = groupLayoutDesc.bindings.emplace_back();
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
                        binding.name, input.name, group.name));
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

        groupLayoutDescs.push_back(groupLayoutDesc);
    }
    
    // Group for inline samplers

    int inlineSamplerGroupIndex = -1;
    if(!input.inlineSamplerDescs.empty())
    {
        RHI::BindingGroupLayoutDesc groupLayoutDesc;
        groupLayoutDesc.bindings =
            {
                RHI::BindingDesc
                {
                    .type              = RHI::BindingType::Sampler,
                    .stages            = ShaderPreprocessingDetail::GetFullShaderStages(category),
                    .arraySize         = static_cast<uint32_t>(input.inlineSamplerDescs.size()),
                    .immutableSamplers = {} // immutableSamplers should be filled by caller
                }
            };
        inlineSamplerGroupIndex = static_cast<int>(groupLayoutDescs.size());
        bindingNameToSlot["_rtrcGeneratedSamplers"] = BindingSlot{ .set = inlineSamplerGroupIndex, .index = 0 };
        groupLayoutDescs.push_back(std::move(groupLayoutDesc));
    }

    // Macros

    std::map<std::string, std::string> macros = input.envir.macros;

    // Keywords

    for(size_t i = 0; i < input.keywords.size(); ++i)
    {
        const int value = input.keywordValues[i].value;
        input.keywords[i].Match(
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

    macros["rtrc_refcode(...)"] = "";
    macros["rtrc_symbol_name(...)"] = "";
    macros["rtrc_vertex(...)"] = "";
    macros["rtrc_vert(...)"] = "";
    macros["rtrc_pixel(...)"] = "";
    macros["rtrc_fragment(...)"] = "";
    macros["rtrc_frag(...)"] = "";
    macros["rtrc_compute(...)"] = "";
    macros["rtrc_comp(...)"] = "";
    macros["rtrc_raytracing(...)"] = "";
    macros["rtrc_rt(...)"] = "";
    macros["rtrc_task(...)"] = "";
    macros["rtrc_mesh(...)"] = "";
    macros["rtrc_rt_group(...)"] = "";
    macros["rtrc_keyword(...)"] = "";

    macros["rtrc_group(NAME, ...)"] = "_rtrc_group_##NAME";
    macros["rtrc_define(TYPE, NAME, ...)"] = "_rtrc_define_##NAME";
    macros["rtrc_alias(TYPE, NAME, ...)"] = "_rtrc_alias_##NAME";
    macros["rtrc_bindless(TYPE, NAME, ...)"] = "_rtrc_define_##NAME";
    macros["rtrc_bindless_varsize(TYPE, NAME, ...)"] = "_rtrc_define_##NAME";
    macros["rtrc_uniform(TYPE, NAME)"] = "";
    macros["rtrc_sampler(NAME, ...)"] = "_rtrc_sampler_##NAME";
    macros["rtrc_ref(NAME, ...)"] = "";
    macros["rtrc_group_struct(NAME, ...)"] = "_rtrc_group_struct_##NAME";
    macros["rtrc_push_constant(NAME, ...)"] = "_rtrc_push_constant_##NAME";

    macros["_rtrc_generated_shader_prefix"]; // Make sure _rtrc_generated_shader_prefix is defined

    bool hasBindless = false;
    std::vector<ShaderUniformBlock> uniformBlocks;
    std::map<std::string, const ParsedBinding *> nameToBinding;
    std::map<std::string, std::string> bindingNameToRegisterSpecifier; // For D3D12 backend

    // Macros for resources and binding groups

    for(auto &group : input.bindingGroups)
    {
        int t = 0, b = 0, s = 0, u = 0;
        std::string groupLeft = fmt::format("_rtrc_group_{}", group.name);
        std::string groupDefinition;

        for(auto &&[bindingIndex, binding] : Enumerate(group.bindings))
        {
            const auto [set, slot] = bindingNameToSlot.at(binding.name);
            nameToBinding[binding.name] = &binding;

            std::string arraySpecifier = ShaderPreprocessingDetail::GetArraySpecifier(binding.arraySize);

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
                using namespace ShaderPreprocessingDetail;

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
                var.type       = GetShaderUniformTypeFromName(uniform.type);
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
        for(auto &range : input.pushConstantRanges)
        {
            for(auto &var : range.variables)
            {
                pushConstantContent += fmt::format(
                    "[[vk::offset({})]] {} {}__{};",
                    var.offset, ParsedPushConstantVariable::TypeToName(var.type), range.name, var.name);
            }
        }
        for(size_t i = 0; i < input.pushConstantRanges.size(); ++i)
        {
            const std::string &name = input.pushConstantRanges[i].name;
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
        const size_t pushConstantRegisterSpace = groupLayoutDescs.size();
        for(auto &&[rangeIndex, range] : Enumerate(input.pushConstantRanges))
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
                inlineSamplerGroupIndex, input.inlineSamplerDescs.size());

            for(auto &[name, index] : input.inlineSamplerNameToDesc)
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
            for(auto &&[i, s] : Enumerate(input.inlineSamplerDescs))
            {
                generatedShaderPrefix += fmt::format(
                    "SamplerState _rtrc_generated_sampler_{} : register(s{}, space{});",
                    i, i, inlineSamplerGroupIndex);
            }
            macros["_rtrc_generated_shader_prefix"] += generatedShaderPrefix;

            for(auto &&[name, index] : input.inlineSamplerNameToDesc)
            {
                std::string left = fmt::format("_rtrc_sampler_{}", name);
                std::string right = fmt::format("static SamplerState {} = _rtrc_generated_sampler_{};", name, index);
                macros.insert({ std::move(left), std::move(right) });
            }
        }
    }

    
    // Macros for ungrouped bindings

    const int groupIndexForUngroupedBindings = static_cast<int>(groupLayoutDescs.size()); // For vulkan backend
    for(auto &&[bindingIndex, binding] : Enumerate(input.ungroupedBindings))
    {
        nameToBinding.insert({ binding.name, &binding });

        std::string templateParamSpec;
        if(!binding.templateParam.empty())
        {
            templateParamSpec = fmt::format("<{}>", binding.templateParam);
        }
        std::string arraySpec = ShaderPreprocessingDetail::GetArraySpecifier(binding.arraySize);

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
            using namespace ShaderPreprocessingDetail;
            assert(backend == RHI::BackendType::DirectX12);
            const char *reg = nullptr;
            switch(BindingTypeToD3D12RegisterType(binding.type))
            {
            case D3D12RegisterType::B: reg = "b"; break;
            case D3D12RegisterType::T: reg = "t"; break;
            case D3D12RegisterType::S: reg = "s"; break;
            case D3D12RegisterType::U: reg = "u"; break;
            }
            assert(reg);
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

    for(auto &&[aliasIndex, alias] : Enumerate(input.aliases))
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
                alias.name, ShaderPreprocessingDetail::GetArraySpecifier(binding.arraySize));
        }
        else
        {
            using namespace ShaderPreprocessingDetail;
            assert(backend == RHI::BackendType::DirectX12);
            const char *reg = nullptr;
            switch(BindingTypeToD3D12RegisterType(alias.bindingType))
            {
            case D3D12RegisterType::B: reg = "b"; break;
            case D3D12RegisterType::T: reg = "t"; break;
            case D3D12RegisterType::S: reg = "s"; break;
            case D3D12RegisterType::U: reg = "u"; break;
            }
            assert(reg);
            right = fmt::format(
                "{}{} {}{} : register({}0, space{});",
                alias.rawTypename, templateParamSpec, alias.name,
                GetArraySpecifier(binding.arraySize),
                reg, 1000 + aliasIndex);
        }
        macros.insert({ std::move(left), std::move(right) });
    }

    // Fill output

    ShaderPreprocessingOutput output;
    output.category = category;

    auto &bindingNameMap = output.bindingNameMap;
    bindingNameMap.allBindingNames_.resize(groupLayoutDescs.size());
    for(size_t i = 0; i < groupLayoutDescs.size(); ++i)
    {
        const size_t bindingCount = groupLayoutDescs[i].bindings.size();
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
    groupNames.resize(input.bindingGroups.size());
    for(auto &&[index, group] : Enumerate(input.bindingGroups))
    {
        nameToGroupIndex[group.name] = static_cast<int>(index);
        groupNames[index] = group.name;
    }
    output.nameToBindingGroupLayoutIndex = std::move(nameToGroupIndex);
    output.groups = std::move(groupLayoutDescs);
    output.bindingGroupNames = std::move(groupNames);

    std::ranges::transform(
        input.pushConstantRanges, std::back_inserter(output.pushConstantRanges),
        [](const ParsedPushConstantRange &range) { return range.range; });
    
    if(backend == RHI::BackendType::DirectX12)
    {
        for(auto &&[aliasIndex, alias] : Enumerate(input.aliases))
        {
            output.unboundedAliases.push_back(RHI::UnboundedBindingArrayAliasing
            {
                .srcGroup = bindingNameToSlot.at(alias.aliasedName).set
            });
        }
    }

    output.name = input.name;
    output.source = input.source;
    output.sourceFilename = input.sourceFilename;
    output.vertexEntry = input.vertexEntry;
    output.fragmentEntry = input.fragmentEntry;
    output.computeEntry = input.computeEntry;
    output.taskEntry = input.taskEntry;
    output.meshEntry = input.meshEntry;
    output.rayTracingEntryGroups = input.entryGroups;

    output.includeDirs = std::move(includeDirs);
    output.macros = std::move(macros);

    output.inlineSamplerBindingGroupIndex = inlineSamplerGroupIndex;
    output.inlineSamplerDescs = input.inlineSamplerDescs;
    output.uniformBlocks = std::move(uniformBlocks);

    output.hasBindless = hasBindless;

    return output;
}

std::set<std::string> GetDependentFiles(
    DXC                             *dxc,
    const ShaderPreprocessingOutput &shader,
    RHI::BackendType                 backend)
{
    Box<DXC> localDXC;
    if(!dxc)
    {
        localDXC = MakeBox<DXC>();
        dxc = localDXC.get();
    }

    DXC::ShaderInfo shaderInfo;
    shaderInfo.source         = shader.source;
    shaderInfo.sourceFilename = shader.sourceFilename;
    shaderInfo.macros         = std::move(shader.macros);
    shaderInfo.includeDirs    = std::move(shader.includeDirs);
    shaderInfo.bindless       = shader.hasBindless;

    if(shader.category == ShaderCategory::RayTracing)
    {
        shaderInfo.rayTracing = true;
        shaderInfo.rayQuery = true;
    }
    else
    {
        auto HasASBinding = [&]
        {
            for(auto &group : shader.groups)
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

    DXC::Target target;
    const bool isVulkan = backend == RHI::BackendType::Vulkan;
    if(shader.category == ShaderCategory::ClassicalGraphics)
    {
        shaderInfo.entryPoint = shader.vertexEntry;
        target = isVulkan ? DXC::Target::Vulkan_1_3_VS_6_6 : DXC::Target::DirectX12_VS_6_6;
    }
    else if(shader.category == ShaderCategory::MeshGraphics)
    {
        shaderInfo.entryPoint = shader.meshEntry;
        target = isVulkan ? DXC::Target::Vulkan_1_3_MS_6_6 : DXC::Target::DirectX12_MS_6_6;
    }
    else if(shader.category == ShaderCategory::Compute)
    {
        shaderInfo.entryPoint = shader.computeEntry;
        target = isVulkan ? DXC::Target::Vulkan_1_3_CS_6_6 : DXC::Target::DirectX12_CS_6_6;
    }
    else
    {
        target = isVulkan ? DXC::Target::Vulkan_1_3_RT_6_6 : DXC::Target::DirectX12_RT_6_6;
    }

    std::string dependencies;
    dxc->Compile(shaderInfo, target, false, nullptr, &dependencies, nullptr);

    const size_t commaPos = dependencies.find(":");
    if(commaPos == std::string::npos)
    {
        throw Exception(fmt::format("Invalid shader dependency info: {}", dependencies));
    }
    const auto lines = Split(dependencies.substr(commaPos + 1), " \\");

    std::set<std::string> ret;
    for(size_t i = 1; i < lines.size(); ++i)
    {
        auto dep = Trim(lines[i]);
        dep = std::filesystem::absolute(dep).lexically_normal().string();
        ret.insert(Replace(dep, "\\", "/"));
    }
    return ret;
}

RTRC_END
