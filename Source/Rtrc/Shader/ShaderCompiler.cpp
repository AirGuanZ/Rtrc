#include <fmt/format.h>

#include <Rtrc/Shader/DXC.h>
#include <Rtrc/Shader/ShaderBindingParser.h>
#include <Rtrc/Shader/ShaderCompiler.h>
#include <Rtrc/Utils/Enumerate.h>
#include <Rtrc/Utils/File.h>
#include <Rtrc/Utils/Unreachable.h>

RTRC_BEGIN

const RHI::RawShaderPtr &Shader::GetRawShader(RHI::ShaderStage stage) const
{
    switch(stage)
    {
    case RHI::ShaderStage::VertexShader:   return VS_;
    case RHI::ShaderStage::FragmentShader: return FS_;
    case RHI::ShaderStage::ComputeShader:  return CS_;
    }
    Unreachable();
}

const RHI::BindingLayoutPtr &Shader::GetRHIBindingLayout() const
{
    return *rhiBindingLayout_;
}

Span<ShaderIOVar> Shader::GetInputVariables() const
{
    return VSRefl_.inputVariables;
}

const RC<BindingGroupLayout> Shader::GetBindingGroupLayoutByName(std::string_view name) const
{
    const int index = GetBindingGroupIndexByName(name);
    return GetBindingGroupLayoutByIndex(index);
}

const RC<BindingGroupLayout> Shader::GetBindingGroupLayoutByIndex(int index) const
{
    return bindingGroupLayouts_[index];
}

int Shader::GetBindingGroupIndexByName(std::string_view name) const
{
    const auto it = nameToBindingGroupLayoutIndex_.find(name);
    if(it == nameToBindingGroupLayoutIndex_.end())
    {
        throw Exception(fmt::format("unknown binding group layout '{}' in shader", name));
    }
    return it->second;
}

void ShaderCompiler::SetDevice(RHI::DevicePtr device)
{
    assert(!rhiDevice_);
    rhiDevice_ = std::move(device);
}

void ShaderCompiler::SetRootDirectory(std::string_view rootDir)
{
    rootDir_ = absolute(std::filesystem::path(rootDir)).lexically_normal();
}

RC<Shader> ShaderCompiler::Compile(const ShaderSource &desc, bool debug, const Macros &macros, bool skipPreprocess)
{
    std::map<std::string, int, std::less<>> nameToGroupIndex;
    std::vector<RC<BindingGroupLayout>>     bindingGroupLayouts;

    RC<Shader> shader = MakeRC<Shader>();

    std::string source = desc.source;
    if(source.empty())
    {
        assert(!desc.filename.empty());
        source = File::ReadTextFile(GetMappedFilename(desc.filename));
    }

    std::string filename = desc.filename;
    if(filename.empty())
    {
        filename = "anonymous.hlsl";
    }

    if(!desc.VSEntry.empty())
    {
        shader->VS_ = CompileShader(
            source, filename, desc.VSEntry, debug, macros, RHI::ShaderStage::VertexShader,
            skipPreprocess, nameToGroupIndex, bindingGroupLayouts, shader->VSRefl_);
    }
    if(!desc.FSEntry.empty())
    {
        shader->FS_ = CompileShader(
            source, filename, desc.FSEntry, debug, macros, RHI::ShaderStage::FragmentShader,
            skipPreprocess, nameToGroupIndex, bindingGroupLayouts, shader->FSRefl_);
    }
    if(!desc.CSEntry.empty())
    {
        shader->CS_ = CompileShader(
            source, filename, desc.CSEntry, debug, macros, RHI::ShaderStage::ComputeShader,
            skipPreprocess, nameToGroupIndex, bindingGroupLayouts, shader->CSRefl_);
    }
    
    shader->nameToBindingGroupLayoutIndex_ = std::move(nameToGroupIndex);
    shader->bindingGroupLayouts_           = std::move(bindingGroupLayouts);

    RHI::BindingLayoutDesc bindingLayoutDesc;
    for(auto &g : shader->bindingGroupLayouts_)
    {
        bindingLayoutDesc.groups.push_back(g->GetRHIBindingGroupLayout());
    }

    shader->rhiBindingLayout_ = bindingLayoutPool_.GetOrCreate(bindingLayoutDesc, [&]
    {
        return rhiDevice_->CreateBindingLayout(bindingLayoutDesc);
    });

    return shader;
}

std::string ShaderCompiler::Preprocess(std::string source, std::string filename, const Macros &macros)
{
    if(source.empty())
    {
        assert(!filename.empty());
        source = File::ReadTextFile(GetMappedFilename(filename));
    }

    if(filename.empty())
    {
        filename = "anonymous.hlsl";
    }

    std::vector<std::string> includeDirs;
    includeDirs.push_back(rootDir_.string());
    if(!filename.empty())
    {
        includeDirs.push_back(
            absolute(std::filesystem::path(filename)).parent_path().lexically_normal().string());
    }

    DXC::ShaderInfo shaderInfo =
    {
        .source         = source,
        .sourceFilename = filename,
        .entryPoint     = "DummyEntry",
        .includeDirs    = includeDirs,
        .macros         = macros
    };

    std::string result;
    DXC().Compile(shaderInfo, DXC::Target::Vulkan_1_3_VS_6_0, true, &result);
    return result;
}

std::string ShaderCompiler::GetMappedFilename(const std::string &filename)
{
    std::filesystem::path path = filename;
    path = path.lexically_normal();
    if(path.is_relative())
    {
        path = rootDir_ / path;
    }
    path = path.lexically_normal();
    return path.string();
}

RHI::RawShaderPtr ShaderCompiler::CompileShader(
    const std::string                               &source,
    const std::string                               &filename,
    const std::string                               &entry,
    bool                                             debug,
    const std::map<std::string, std::string>        &macros,
    RHI::ShaderStage                                 stage,
    bool                                             skipPreprocess,
    std::map<std::string, int, std::less<>>         &outputNameToGroupIndex,
    std::vector<RC<BindingGroupLayout>>             &outBindingGroupLayouts,
    ShaderReflection                                &outputRefl)
{
    DXC::Target target = {};
    switch(stage)
    {
    case RHI::ShaderStage::VertexShader:   target = DXC::Target::Vulkan_1_3_VS_6_0; break;
    case RHI::ShaderStage::FragmentShader: target = DXC::Target::Vulkan_1_3_FS_6_0; break;
    case RHI::ShaderStage::ComputeShader:  target = DXC::Target::Vulkan_1_3_CS_6_0; break;
    }

    assert(!source.empty() && !filename.empty());

    // preprocess

    std::vector<std::string> includeDirs;
    includeDirs.push_back(rootDir_.string());
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

    std::string preprocessedSource;
    if(skipPreprocess)
    {
        preprocessedSource = source;
    }
    else
    {
        dxc.Compile(shaderInfo, target, debug, &preprocessedSource);
    }

    // parse binding groups

    std::vector<ShaderBindingGroupRewrite::ParsedBindingGroup> parsedBindingGroups;
    while(true)
    {
        ShaderBindingGroupRewrite::ParsedBindingGroup group;
        if(RewriteNextBindingGroup(preprocessedSource, group))
        {
            parsedBindingGroups.push_back(group);
        }
        else
        {
            break;
        }
    }

    // parse bindings

    std::vector<ShaderBindingParse::ParsedBinding> parsedBindings;
    ParseBindings(preprocessedSource, parsedBindings);

    // map binding name to (group, slot)

    struct BindingRecord
    {
        int groupIndex;
        int slotIndex;
        const ShaderBindingParse::ParsedBinding *parsedBinding;
    };

    std::map<std::string, BindingRecord> bindingNameToRecord;
    for(auto &&[groupIndex, group] : Enumerate(parsedBindingGroups))
    {
        for(auto &&[slotIndex, aliasedBindings] : Enumerate(group.bindings))
        {
            for(auto &binding : aliasedBindings)
            {
                if(bindingNameToRecord.contains(binding.name))
                {
                    throw Exception(fmt::format(
                        "binding name {} appears in more than one binding groups", binding.name));
                }
                bindingNameToRecord[binding.name] = {
                    static_cast<int>(groupIndex),
                    static_cast<int>(slotIndex),
                    nullptr
                };
            }
        }
    }

    // register grouped bindings, and remove ungrouped ones

    for(auto &binding : parsedBindings)
    {
        auto it = bindingNameToRecord.find(binding.name);
        if(it != bindingNameToRecord.end())
        {
            it->second.parsedBinding = &binding;
        }
        else
        {
            for(size_t i = binding.begPosInSource; i < binding.endPosInSource; ++i)
            {
                char &c = preprocessedSource[i];
                if(!std::isspace(c))
                {
                    c = ' ';
                }
            }
        }
    }

    // generate groups

    std::map<std::string, std::pair<int, int>> bindingNameToFinalSlots;

    for(auto &group : parsedBindingGroups)
    {
        RHI::BindingGroupLayoutDesc rhiLayoutDesc;
        rhiLayoutDesc.name = group.name;

        for(auto &aliasedBindings : group.bindings)
        {
            RHI::AliasedBindingsDesc rhiAliasedBindingsDesc;
            for(auto &binding : aliasedBindings)
            {
                auto it = bindingNameToRecord.find(binding.name);
                if(it == bindingNameToRecord.end() || !it->second.parsedBinding)
                {
                    throw Exception(fmt::format("binding {} in group {} is not defined", binding.name, group.name));
                }
                const BindingRecord &record = it->second;

                RHI::BindingDesc rhiBindingDesc;
                rhiBindingDesc.type         = record.parsedBinding->type;
                rhiBindingDesc.shaderStages = binding.stages;
                rhiBindingDesc.arraySize    = record.parsedBinding->arraySize;
                rhiAliasedBindingsDesc.push_back(rhiBindingDesc);
            }
            rhiLayoutDesc.bindings.push_back(rhiAliasedBindingsDesc);
        }

        if(auto it = outputNameToGroupIndex.find(group.name); it != outputNameToGroupIndex.end())
        {
            if(rhiLayoutDesc != outBindingGroupLayouts[it->second]->rhiLayout_->GetDesc())
            {
                throw Exception("binding group {} appears in multiple stages with different definitions");
            }

            for(auto &&[slotIndex, aliasedBindings] : Enumerate(group.bindings))
            {
                for(auto &binding : aliasedBindings)
                {
                    bindingNameToFinalSlots[binding.name] = { it->second, static_cast<int>(slotIndex) };
                }
            }
        }
        else
        {
            auto groupLayout = bindingGroupLayoutPool_.GetOrCreate(rhiLayoutDesc, [&]
            {
                auto layout = MakeRC<BindingGroupLayout>();
                layout->groupName_ = group.name;
                layout->rhiLayout_ = rhiDevice_->CreateBindingGroupLayout(rhiLayoutDesc);
                for(auto &&[slot, aliasedBindings] : Enumerate(group.bindings))
                {
                    for(auto &binding : aliasedBindings)
                    {
                        layout->bindingNameToSlot_[binding.name] = static_cast<int>(slot);
                    }
                }
                return layout;
            });

            const int groupIndex = static_cast<int>(outBindingGroupLayouts.size());

            outputNameToGroupIndex[group.name] = groupIndex;
            outBindingGroupLayouts.push_back(groupLayout);

            for(auto &&[slotIndex, aliasedBindings] : Enumerate(group.bindings))
            {
                for(auto &binding : aliasedBindings)
                {
                    bindingNameToFinalSlots[binding.name] = { groupIndex, static_cast<int>(slotIndex) };
                }
            }
        }
    }

    // rewrite definitions

    std::sort(
        parsedBindings.begin(), parsedBindings.end(),
        [](const ShaderBindingParse::ParsedBinding &a, const ShaderBindingParse::ParsedBinding &b)
    {
        return a.begPosInSource > b.begPosInSource;
    });

    for(auto &parsedBinding : parsedBindings)
    {
        auto it = bindingNameToFinalSlots.find(parsedBinding.name);
        if(it == bindingNameToFinalSlots.end())
        {
            continue;
        }

        const int set = it->second.first;
        const int slot = it->second.second;
        const std::string bindingDeclaration = fmt::format("[[vk::binding({}, {})]] ", slot, set);
        preprocessedSource.insert(parsedBinding.begPosInSource, bindingDeclaration);
    }

    shaderInfo.source = preprocessedSource;
    const std::vector<unsigned char> compileData = dxc.Compile(shaderInfo, target, debug, nullptr);

    ReflectSPIRV(compileData, entry.c_str(), outputRefl);
    return rhiDevice_->CreateShader(compileData.data(), compileData.size(), entry, stage);
}

RTRC_END
