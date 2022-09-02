#include <fmt/format.h>

#include <Rtrc/Shader/DXC.h>
#include <Rtrc/Shader/ShaderBindingParser.h>
#include <Rtrc/Shader/ShaderCompiler.h>
#include <Rtrc/Utils/Enumerate.h>
#include <Rtrc/Utils/File.h>
#include <Rtrc/Utils/Unreachable.h>

RTRC_BEGIN

Shader::~Shader()
{
    if(parentManager_)
    {
        parentManager_->OnShaderDestroyed(bindingGroupLayoutIterators_, bindingLayoutIterator_);
    }
}

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
    return bindingLayoutIterator_->second.layout;
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

Shader::BindingGroupLayoutRecord::BindingGroupLayoutRecord(const BindingGroupLayoutRecord &other)
    : layout(other.layout), shaderCounter(other.shaderCounter)
{
    
}

Shader::BindingGroupLayoutRecord &Shader::BindingGroupLayoutRecord::operator=(const BindingGroupLayoutRecord &other)
{
    if(this != &other)
    {
        layout = other.layout;
        shaderCounter = other.shaderCounter;
    }
    return *this;
}

ShaderCompiler::ShaderCompiler(RHI::DevicePtr device)
    : rhiDevice_(std::move(device)), debug_(RTRC_DEBUG)
{
    
}

void ShaderCompiler::SetDevice(RHI::DevicePtr device)
{
    assert(!rhiDevice_);
    rhiDevice_ = std::move(device);
}

void ShaderCompiler::SetDebugMode(bool enableDebug)
{
    debug_ = enableDebug;
}

void ShaderCompiler::SetFileLoader(std::string_view rootDir)
{
    rootDir_ = absolute(std::filesystem::path(rootDir)).lexically_normal();
}

RC<Shader> ShaderCompiler::Compile(const ShaderDescription &desc)
{
    bool debug = debug_;
    if(desc.overrideDebugMode.has_value())
    {
        debug = desc.overrideDebugMode.value();
    }

    std::map<std::string, int, std::less<>>         nameToGroupIndex;
    std::vector<RC<BindingGroupLayout>>             bindingGroupLayouts;
    std::vector<Shader::BindingGroupLayoutRecordIt> bindingGroupLayoutIterators;

    RC<Shader> shader = MakeRC<Shader>();
    
    if(!desc.VS.filename.empty() || !desc.VS.source.empty())
    {
        shader->VS_ = CompileShader(
            debug, desc.macros, desc.VS, RHI::ShaderStage::VertexShader,
            nameToGroupIndex, bindingGroupLayouts, bindingGroupLayoutIterators, shader->VSRefl_);
    }
    if(!desc.FS.filename.empty() || !desc.FS.source.empty())
    {
        shader->FS_ = CompileShader(
            debug, desc.macros, desc.FS, RHI::ShaderStage::FragmentShader,
            nameToGroupIndex, bindingGroupLayouts, bindingGroupLayoutIterators, shader->FSRefl_);
    }
    if(!desc.CS.filename.empty() || !desc.CS.source.empty())
    {
        shader->CS_ = CompileShader(
            debug, desc.macros, desc.CS, RHI::ShaderStage::ComputeShader,
            nameToGroupIndex, bindingGroupLayouts, bindingGroupLayoutIterators, shader->CSRefl_);
    }

    shader->parentManager_ = this;
    shader->nameToBindingGroupLayoutIndex_ = std::move(nameToGroupIndex);
    shader->bindingGroupLayouts_           = std::move(bindingGroupLayouts);
    shader->bindingGroupLayoutIterators_   = std::move(bindingGroupLayoutIterators);

    RHI::BindingLayoutDesc bindingLayoutDesc;
    for(auto &g : shader->bindingGroupLayouts_)
    {
        bindingLayoutDesc.groups.push_back(g->GetRHIBindingGroupLayout());
    }
    if(auto it = descToBindingLayout_.find(bindingLayoutDesc); it != descToBindingLayout_.end())
    {
        shader->bindingLayoutIterator_ = it;
        ++it->second.shaderCounter;
    }
    else
    {
        auto layout = rhiDevice_->CreateBindingLayout(bindingLayoutDesc);
        shader->bindingLayoutIterator_ = descToBindingLayout_.insert({ bindingLayoutDesc, { layout, 1 } }).first;
    }

    return shader;
}

const RC<BindingGroupLayout> &ShaderCompiler::GetBindingGroupLayoutByName(std::string_view name) const
{
    const auto it = nameToBindingGroupLayout_.find(name);
    if(it == nameToBindingGroupLayout_.end())
    {
        throw Exception(fmt::format("binding group '{}' is not found", name));
    }
    if(!it->second)
    {
        throw Exception(fmt::format("there are multiple binding groups sharing the same name '{}'", name));
    }
    return it->second;
}

void ShaderCompiler::OnShaderDestroyed(
    std::vector<Shader::BindingGroupLayoutRecordIt> &its, Shader::BindingLayoutRecordIt itt)
{
    for(auto &it : its)
    {
        if(!--it->second.shaderCounter)
        {
            if(auto sit = nameToBindingGroupLayout_.find(it->first.name); sit != nameToBindingGroupLayout_.end())
            {
                if(sit->second == it->second.layout)
                {
                    nameToBindingGroupLayout_.erase(sit);
                }
            }
            descToBindingGroupLayout_.erase(it);
        }
    }

    if(!--itt->second.shaderCounter)
    {
        descToBindingLayout_.erase(itt);
    }
}

RHI::RawShaderPtr ShaderCompiler::CompileShader(
    bool                                             debug,
    const std::map<std::string, std::string>        &macros,
    const ShaderSource                              &source,
    RHI::ShaderStage                                 stage,
    std::map<std::string, int, std::less<>>         &outputNameToGroupIndex,
    std::vector<RC<BindingGroupLayout>>             &bindingGroupLayouts,
    std::vector<Shader::BindingGroupLayoutRecordIt> &outputBindingGroupLayouts,
    ShaderReflection                                &outputRefl)
{
    DXC::Target target = {};
    switch(stage)
    {
    case RHI::ShaderStage::VertexShader:   target = DXC::Target::Vulkan_1_3_VS_6_0; break;
    case RHI::ShaderStage::FragmentShader: target = DXC::Target::Vulkan_1_3_FS_6_0; break;
    case RHI::ShaderStage::ComputeShader:  target = DXC::Target::Vulkan_1_3_CS_6_0; break;
    }

    // load source

    std::string sourceFilename;
    if (!source.filename.empty())
    {
        sourceFilename = GetMappedFilename(source.filename);
    }

    std::string rawSource;
    if(!source.source.empty())
    {
        rawSource = source.source;
    }
    else
    {
        assert(!sourceFilename.empty());
        rawSource = File::ReadTextFile(sourceFilename);
    }

    // preprocess

    std::vector<std::string> includeDirs;
    includeDirs.push_back(rootDir_.string());
    if(!sourceFilename.empty())
    {
        includeDirs.push_back(
            absolute(std::filesystem::path(sourceFilename)).parent_path().lexically_normal().string());
    }

    std::string preprocessedSource;
    DXC::ShaderInfo shaderInfo =
    {
        .source         = rawSource,
        .sourceFilename = sourceFilename,
        .entryPoint     = source.entry,
        .includeDirs    = includeDirs,
        .macros         = macros
    };
    DXC dxc;
    dxc.Compile(shaderInfo, target, debug, &preprocessedSource);

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
            if(rhiLayoutDesc != bindingGroupLayouts[it->second]->rhiLayout_->GetDesc())
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
            auto recordIt = descToBindingGroupLayout_.find(rhiLayoutDesc);
            if(recordIt == descToBindingGroupLayout_.end())
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

                Shader::BindingGroupLayoutRecord record;
                record.layout = std::move(layout);
                record.shaderCounter = 1;
                recordIt = descToBindingGroupLayout_.insert({ rhiLayoutDesc, record }).first;

                if(auto sit = nameToBindingGroupLayout_.find(group.name); sit != nameToBindingGroupLayout_.end())
                {
                    if(sit->second != recordIt->second.layout)
                    {
                        sit->second = nullptr;
                    }
                }
                else
                {
                    nameToBindingGroupLayout_[group.name] = recordIt->second.layout;
                }
            }
            else
            {
                ++recordIt->second.shaderCounter;
            }

            const int groupIndex = static_cast<int>(bindingGroupLayouts.size());

            outputNameToGroupIndex[group.name] = groupIndex;
            bindingGroupLayouts.push_back(recordIt->second.layout);
            outputBindingGroupLayouts.push_back(recordIt);

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

    if(source.dumpedPreprocessedSource)
    {
        *source.dumpedPreprocessedSource = preprocessedSource;
    }

    shaderInfo.source = preprocessedSource;
    const std::vector<unsigned char> compileData = dxc.Compile(shaderInfo, target, debug, nullptr);

    ReflectSPIRV(compileData, source.entry.c_str(), outputRefl);
    return rhiDevice_->CreateShader(compileData.data(), compileData.size(), source.entry, stage);
}

std::string ShaderCompiler::GetMappedFilename(const std::string &filename)
{
    std::filesystem::path path = filename;
    path = path.lexically_normal();
    if (path.is_relative())
    {
        path = rootDir_ / path;
    }
    path = path.lexically_normal();
    return path.string();
}

RTRC_END
