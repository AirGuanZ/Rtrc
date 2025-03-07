#include <Rtrc/Core/Enumerate.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Shader/Compiler.h>
#include <Rtrc/Graphics/Shader/ShaderBuilder.h>
#include <Rtrc/ShaderCommon/Reflection/D3D12Reflection.h>
#include <Rtrc/ShaderCommon/Reflection/SPIRVReflection.h>

RTRC_BEGIN

void ShaderCompiler::SetDevice(Ref<Device> device)
{
    device_ = device;
}

RC<Shader> ShaderCompiler::Compile(
    const CompilableShader &shader,
    bool                    debug) const
{
    assert(device_);
    const RHI::BackendType backend = device_->GetBackendType();
    auto preprocessingOutput = PreprocessShader(shader, backend);

    // Compile

    const bool VS = !shader.vertexEntry.empty();
    const bool FS = !shader.fragmentEntry.empty();
    const bool CS = !shader.computeEntry.empty();
    const bool RT = shader.isRayTracingShader;
    const bool TS = !shader.taskEntry.empty();
    const bool MS = !shader.meshEntry.empty();
    const bool WG = !shader.workGraphEntryNodes.empty();

    DXC::ShaderInfo shaderInfo;
    shaderInfo.source         = shader.source;
    shaderInfo.sourceFilename = shader.sourceFilename;
    shaderInfo.macros         = std::move(preprocessingOutput.macros);
    shaderInfo.includeDirs    = std::move(preprocessingOutput.includeDirs);
    shaderInfo.bindless       = preprocessingOutput.hasBindless;

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

    std::vector<unsigned char> vsData, fsData, csData, rtData, tsData, msData, wgData;
    Box<ShaderReflection> vsRefl, fsRefl, csRefl, rtRefl, tsRefl, msRefl, wgRefl;
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
    if(WG)
    {
        DoCompilation(shaderInfo, CompileStage::WG, debug, wgData, wgRefl);
    }
    if(TS)
    {
        shaderInfo.entryPoint = shader.taskEntry;
        DoCompilation(shaderInfo, CompileStage::TS, debug, tsData, tsRefl);
    }
    if(MS)
    {
        shaderInfo.entryPoint = shader.meshEntry;
        DoCompilation(shaderInfo, CompileStage::MS, debug, msData, msRefl);
    }

    // Check uses of ungrouped bindings

    EnsureAllUsedBindingsAreGrouped(shader, vsRefl, "vertex shader");
    EnsureAllUsedBindingsAreGrouped(shader, fsRefl, "fragment shader");
    EnsureAllUsedBindingsAreGrouped(shader, csRefl, "compute shader");
    EnsureAllUsedBindingsAreGrouped(shader, rtRefl, "ray tracing shader");
    EnsureAllUsedBindingsAreGrouped(shader, tsRefl, "task shader");
    EnsureAllUsedBindingsAreGrouped(shader, msRefl, "mesh shader");
    EnsureAllUsedBindingsAreGrouped(shader, wgRefl, "work graph shader");

    // Binding layout

    std::vector<RC<BindingGroupLayout>> bindingGroupLayouts;
    for(auto &&[groupIndex, group] : Enumerate(preprocessingOutput.groups))
    {
        BindingGroupLayout::Desc desc;
        desc.bindings.reserve(group.bindings.size());
        for(auto &binding : group.bindings)
        {
            desc.bindings.push_back(BindingGroupLayout::BindingDesc
            {
                .type              = binding.type,
                .stages            = binding.stages,
                .arraySize         = binding.arraySize,
                .immutableSamplers = {},
                .bindless          = binding.bindless
            });
        }
        desc.variableArraySize = group.variableArraySize;

        if(preprocessingOutput.inlineSamplerBindingGroupIndex >= 0 &&
           static_cast<int>(groupIndex) == preprocessingOutput.inlineSamplerBindingGroupIndex)
        {
            auto &samplers = desc.bindings[0].immutableSamplers;
            samplers.reserve(preprocessingOutput.inlineSamplerDescs.size());
            for(auto &samplerDesc : preprocessingOutput.inlineSamplerDescs)
            {
                samplers.push_back(device_->CreateSampler(samplerDesc));
            }
        }

        bindingGroupLayouts.push_back(device_->CreateBindingGroupLayout(desc));
    }

    RC<BindingLayout> bindingLayout;
    {
        BindingLayout::Desc desc;
        desc.groupLayouts       = bindingGroupLayouts;
        desc.pushConstantRanges = preprocessingOutput.pushConstantRanges;
        desc.unboundedAliases   = preprocessingOutput.unboundedAliases;
        bindingLayout = device_->CreateBindingLayout(desc);
    }

    // Fill shader object

    ShaderBuilder::Desc desc;
    desc.category = preprocessingOutput.category;

    if(VS)
    {
        desc.vertexShader = device_->GetRawDevice()->CreateShader(
            vsData.data(), vsData.size(), { { RHI::ShaderStage::VertexShader, shader.vertexEntry } });
        desc.VSInput = vsRefl->GetInputVariables();
    }
    if(FS)
    {
        desc.fragmentShader = device_->GetRawDevice()->CreateShader(
            fsData.data(), fsData.size(), { { RHI::ShaderStage::FragmentShader, shader.fragmentEntry } });
    }
    if(CS)
    {
        desc.computeShader = device_->GetRawDevice()->CreateShader(
            csData.data(), csData.size(), { { RHI::ShaderStage::ComputeShader, shader.computeEntry } });
    }
    if(TS)
    {
        desc.taskShader = device_->GetRawDevice()->CreateShader(
            tsData.data(), tsData.size(), { { RHI::ShaderStage::TaskShader, shader.taskEntry } });
    }
    if(MS)
    {
        desc.meshShader = device_->GetRawDevice()->CreateShader(
            msData.data(), msData.size(), { { RHI::ShaderStage::MeshShader, shader.meshEntry } });
    }
    if(WG)
    {
        desc.workGraphShader = device_->GetRawDevice()->CreateShader(wgData.data(), wgData.size(), {});
    }
    if(RT)
    {
        std::vector<RHI::RawShaderEntry> rtEntries = rtRefl->GetEntries();
        auto FindEntryIndex = [&rtEntries](std::string_view name)
        {
            for(uint32_t i = 0; i < rtEntries.size(); ++i)
            {
                if(rtEntries[i].name == name)
                {
                    return i;
                }
            }
            throw Exception(std::format("Ray tracing entry {} is required by a shader group but not found", name));
        };

        for(const std::vector<std::string> &rawGroup : shader.rayTracingEntryGroups)
        {
            assert(!rawGroup.empty());
            const uint32_t firstEntryIndex = FindEntryIndex(rawGroup[0]);
            if(rtEntries[firstEntryIndex].stage == RHI::ShaderStage::RT_RayGenShader)
            {
                if(rawGroup.size() != 1)
                {
                    throw Exception("RayGen shader group must contains no more than one shader");
                }
                desc.rayGenShaderGroups.push_back({ firstEntryIndex });
            }
            else if(rtEntries[firstEntryIndex].stage == RHI::ShaderStage::RT_MissShader)
            {
                if(rawGroup.size() != 1)
                {
                    throw Exception("Miss shader group must contains no more than one shader");
                }
                desc.missShaderGroups.push_back({ firstEntryIndex });
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
                        throw Exception(std::format(
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
                desc.hitShaderGroups.push_back(group);
            }
        }

        desc.rayTracingShader = device_->GetRawDevice()->CreateShader(
            rtData.data(), rtData.size(), std::move(rtEntries));
    }

    if(WG)
    {
        desc.workGraphEntryNodes = shader.workGraphEntryNodes;
    }

    desc.nameToBindingGroupLayoutIndex = std::move(preprocessingOutput.nameToBindingGroupLayoutIndex);
    desc.bindingGroupLayouts           = std::move(bindingGroupLayouts);
    desc.bindingGroupNames             = std::move(preprocessingOutput.bindingGroupNames);
    desc.bindingLayout                 = std::move(bindingLayout);
    desc.bindingNameMap                = std::move(preprocessingOutput.bindingNameMap);

    {
        using enum Shader::BuiltinBindingGroup;
        auto SetBuiltinBindingGroupIndex = [&](Shader::BuiltinBindingGroup group, std::string_view name)
        {
            const auto it = desc.nameToBindingGroupLayoutIndex.find(name);
            const int index = it != desc.nameToBindingGroupLayoutIndex.end() ? it->second : -1;
            desc.builtinBindingGroupIndices[std::to_underlying(group)] = index;
        };
        SetBuiltinBindingGroupIndex(Pass,                   "Pass");
        SetBuiltinBindingGroupIndex(Material,               "Material");
        SetBuiltinBindingGroupIndex(Object,                 "Object");
        SetBuiltinBindingGroupIndex(BindlessTexture,        "GlobalBindlessTextureGroup");
        SetBuiltinBindingGroupIndex(BindlessGeometryBuffer, "GlobalBindlessGeometryBufferGroup");
    }

    desc.pushConstantRanges = std::move(preprocessingOutput.pushConstantRanges);
    
    if(preprocessingOutput.inlineSamplerBindingGroupIndex >= 0)
    {
        desc.bindingGroupForInlineSamplers =
            desc.bindingGroupLayouts[preprocessingOutput.inlineSamplerBindingGroupIndex]->CreateBindingGroup();
    }

    if(CS)
    {
        desc.computeShaderThreadGroupSize = csRefl->GetComputeShaderThreadGroupSize();
    }

    return ShaderBuilder::BuildShader(device_, std::move(desc));
}

void ShaderCompiler::DoCompilation(
    const DXC::ShaderInfo      &shaderInfo,
    CompileStage                stage,
    bool                        debug,
    std::vector<unsigned char> &outData,
    Box<ShaderReflection>      &outRefl) const
{
    DXC::Target target = DXC::Target::Vulkan_1_3_CS_6_8;
    const bool isVulkan = device_->GetBackendType() == RHI::BackendType::Vulkan;
    switch(stage)
    {
    case CompileStage::VS: target = isVulkan ? DXC::Target::Vulkan_1_3_VS_6_8 : DXC::Target::DirectX12_VS_6_8; break;
    case CompileStage::FS: target = isVulkan ? DXC::Target::Vulkan_1_3_FS_6_8 : DXC::Target::DirectX12_FS_6_8; break;
    case CompileStage::CS: target = isVulkan ? DXC::Target::Vulkan_1_3_CS_6_8 : DXC::Target::DirectX12_CS_6_8; break;
    case CompileStage::RT: target = isVulkan ? DXC::Target::Vulkan_1_3_RT_6_8 : DXC::Target::DirectX12_RT_6_8; break;
    case CompileStage::TS: target = isVulkan ? DXC::Target::Vulkan_1_3_TS_6_8 : DXC::Target::DirectX12_TS_6_8; break;
    case CompileStage::MS: target = isVulkan ? DXC::Target::Vulkan_1_3_MS_6_8 : DXC::Target::DirectX12_MS_6_8; break;
    case CompileStage::WG: target = isVulkan ? DXC::Target::Vulkan_1_3_WG_6_8 : DXC::Target::DirectX12_WG_6_8; break;
    }

    if(isVulkan)
    {
        outData = dxc_.Compile(shaderInfo, target, debug, nullptr, nullptr, nullptr);
        outRefl = MakeBox<SPIRVReflection>(outData, shaderInfo.entryPoint);
    }
    else
    {
        assert(device_->GetBackendType() == RHI::BackendType::DirectX12);
        std::vector<std::byte> reflData;
        outData = dxc_.Compile(shaderInfo, target, debug, nullptr, nullptr, &reflData);
        outRefl = MakeBox<D3D12Reflection>(
            dxc_.GetDxcUtils(), reflData,
            target == DXC::Target::DirectX12_RT_6_8 || target == DXC::Target::DirectX12_WG_6_8);
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
                throw Exception(std::format(
                    "Binding {} is not explicitly grouped but is used in {}", binding.name, stage));
            }
        }
    }
}

RTRC_END
