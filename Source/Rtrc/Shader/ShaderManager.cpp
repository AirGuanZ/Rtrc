#include <filesystem>
#include <fstream>
#include <iterator>

#include <fmt/format.h>

#include <Rtrc/Shader/DXC.h>
#include <Rtrc/Shader/ShaderBindingParser.h>
#include <Rtrc/Shader/ShaderManager.h>
#include <Rtrc/Utils/Enumerate.h>
#include <Rtrc/Utils/File.h>
#include <Rtrc/Utils/ScopeGuard.h>
#include <Rtrc/Utils/TemporalFile.h>
#include <Rtrc/Utils/Unreachable.h>

RTRC_BEGIN

namespace
{

    class DefaultShaderFileLoader : public ShaderFileLoader
    {
    public:

        bool Load(std::string_view filename, std::string &output) const override
        {
            const std::string path = GetMappedSourceFilename(filename);
            try
            {
                output = File::ReadTextFile(path);
                return true;
            }
            catch(...)
            {
                return false;
            }
        }

        std::string GetMappedSourceFilename(std::string_view filename) const
        {
            std::filesystem::path path = filename;
            path = path.lexically_normal();
            if(path.is_relative())
            {
                path = rootDir / path;
            }
            path = path.lexically_normal();
            return path.string();
        }

        std::filesystem::path rootDir;
    };

    class DXCIncludeHandlerWrapper : public DXC::IncludeHandler
    {
    public:

        bool Handle(const std::string &filename, std::string &output) const override
        {
            return loader->Load(filename, output);
        }

        RC<ShaderFileLoader> loader;
    };

} // namespace anonymouse

void ParsedBindingGroupLayout::AppendSlot(Span<ParsedBindingDescription> aliasedDescs)
{
    const uint32_t offset = static_cast<uint32_t>(allDescs_.size());
    allDescs_.reserve(allDescs_.size() + aliasedDescs.size());
    std::copy(aliasedDescs.begin(), aliasedDescs.end(), std::back_inserter(allDescs_));
    slotRecords_.push_back({ offset, aliasedDescs.GetSize() });
}

int ParsedBindingGroupLayout::GetBindingSlotCount() const
{
    return static_cast<int>(slotRecords_.size());
}

Span<ParsedBindingGroupLayout::ParsedBindingDescription>
    ParsedBindingGroupLayout::GetBindingDescriptionsBySlot(int slot) const
{
    const Record &record = slotRecords_[slot];
    return Span(&allDescs_[record.offset], record.count);
}

BindingGroup::BindingGroup(const BindingGroupLayout *parentLayout, RHI::BindingGroupPtr rhiGroup)
    : parentLayout_(parentLayout), rhiGroup_(std::move(rhiGroup))
{
    
}

void BindingGroup::Set(int slot, const RHI::BufferPtr &cbuffer, size_t offset, size_t bytes)
{
    rhiGroup_->ModifyMember(slot, cbuffer, offset, bytes);
}

void BindingGroup::Set(int slot, const RHI::SamplerPtr &sampler)
{
    rhiGroup_->ModifyMember(slot, sampler);
}

void BindingGroup::Set(int slot, const RHI::BufferSRVPtr &srv)
{
    rhiGroup_->ModifyMember(slot, srv);
}

void BindingGroup::Set(int slot, const RHI::BufferUAVPtr &uav)
{
    rhiGroup_->ModifyMember(slot, uav);
}

void BindingGroup::Set(int slot, const RHI::Texture2DSRVPtr &srv)
{
    rhiGroup_->ModifyMember(slot, srv);
}

void BindingGroup::Set(int slot, const RHI::Texture2DUAVPtr &uav)
{
    rhiGroup_->ModifyMember(slot, uav);
}

void BindingGroup::Set(std::string_view name, const RHI::BufferPtr &cbuffer, size_t offset, size_t bytes)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, cbuffer, offset, bytes);
}

void BindingGroup::Set(std::string_view name, const RHI::SamplerPtr &sampler)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, sampler);
}

void BindingGroup::Set(std::string_view name, const RHI::BufferSRVPtr &srv)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, srv);
}

void BindingGroup::Set(std::string_view name, const RHI::BufferUAVPtr &uav)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, uav);
}

void BindingGroup::Set(std::string_view name, const RHI::Texture2DSRVPtr &srv)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, srv);
}

void BindingGroup::Set(std::string_view name, const RHI::Texture2DUAVPtr &uav)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, uav);
}

RHI::BindingGroupPtr BindingGroup::GetRHIBindingGroup()
{
    return rhiGroup_;
}

const std::string &BindingGroupLayout::GetGroupName() const
{
    return groupName_;
}

int BindingGroupLayout::GetBindingSlotByName(std::string_view bindingName) const
{
    const auto it = bindingNameToSlot_.find(bindingName);
    if(it == bindingNameToSlot_.end())
    {
        throw Exception(fmt::format("unknown binding name '{}' in group '{}'", bindingName, groupName_));
    }
    return it->second;
}

RHI::BindingGroupLayoutPtr BindingGroupLayout::GetRHIBindingGroupLayout()
{
    return rhiLayout_;
}

RC<BindingGroup> BindingGroupLayout::CreateBindingGroup() const
{
    RHI::BindingGroupPtr rhiGroup = rhiLayout_->CreateBindingGroup();
    return MakeRC<BindingGroup>(this, std::move(rhiGroup));
}

Shader::~Shader()
{
    assert(parentManager_);
    parentManager_->OnShaderDestroyed(bindingGroupLayoutIterators_, bindingLayoutIterator_);
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

ShaderManager::ShaderManager(RHI::DevicePtr device)
    : rhiDevice_(std::move(device)), debug_(RTRC_DEBUG)
{
    
}

void ShaderManager::SetDevice(RHI::DevicePtr device)
{
    assert(!rhiDevice_);
    rhiDevice_ = std::move(device);
}

void ShaderManager::SetDebugMode(bool enableDebug)
{
    debug_ = enableDebug;
}

void ShaderManager::SetFileLoader(std::string_view rootDir)
{
    rootDir_ = absolute(std::filesystem::path(rootDir)).lexically_normal();
}

void ShaderManager::AddMacro(std::string key, std::string value)
{
    macros_[std::move(key)] = std::move(value);
}

RC<Shader> ShaderManager::AddShader(const ShaderDescription &desc)
{
    std::map<std::string, std::string> macros = macros_;
    for(auto &[k, v] : desc.overrideMacros)
    {
        macros_[k] = v;
    }

    bool debug = debug_;
    if(desc.overrideDebugMode.has_value())
    {
        debug = desc.overrideDebugMode.value();
    }

    std::map<std::string, int, std::less<>>         nameToGroupIndex;
    std::vector<RC<BindingGroupLayout>>             bindingGroupLayouts;
    std::vector<Shader::BindingGroupLayoutRecordIt> bindingGroupLayoutIterators;

    RHI::RawShaderPtr VS, FS, CS;
    if(!desc.VS.filename.empty() || !desc.VS.source.empty())
    {
        VS = CompileShader(
            debug, macros, desc.VS, RHI::ShaderStage::VertexShader,
            nameToGroupIndex, bindingGroupLayouts, bindingGroupLayoutIterators);
    }
    if(!desc.FS.filename.empty() || !desc.FS.source.empty())
    {
        FS = CompileShader(
            debug, macros, desc.FS, RHI::ShaderStage::FragmentShader,
            nameToGroupIndex, bindingGroupLayouts, bindingGroupLayoutIterators);
    }
    if(!desc.CS.filename.empty() || !desc.CS.source.empty())
    {
        CS = CompileShader(
            debug, macros, desc.CS, RHI::ShaderStage::ComputeShader,
            nameToGroupIndex, bindingGroupLayouts, bindingGroupLayoutIterators);
    }

    RC<Shader> shader = MakeRC<Shader>();
    shader->parentManager_ = this;
    shader->VS_ = VS;
    shader->FS_ = FS;
    shader->CS_ = CS;
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

const RC<BindingGroupLayout> &ShaderManager::GetBindingGroupLayoutByName(std::string_view name) const
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

void ShaderManager::OnShaderDestroyed(
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

RHI::RawShaderPtr ShaderManager::CompileShader(
    bool                                             debug,
    const std::map<std::string, std::string>        &macros,
    const ShaderSource                              &source,
    RHI::ShaderStage                                 stage,
    std::map<std::string, int, std::less<>>         &outputNameToGroupIndex,
    std::vector<RC<BindingGroupLayout>>             &bindingGroupLayouts,
    std::vector<Shader::BindingGroupLayoutRecordIt> &outputBindingGroupLayouts)
{
    // When vulkan-with-source is enabled, dxc will always try to load source content from `sourceFilename`,
    // instead of using `source` directly. This can cause problems when file `sourceFilename` is not available or
    // its content doesn't strictly match `source`. We work around this problem by creating a temporary source file.
    const bool needTempSourceFile = !source.source.empty() && !source.filename.empty();

    auto fileLoader = MakeRC<DefaultShaderFileLoader>();
    fileLoader->rootDir = rootDir_;

    DXCIncludeHandlerWrapper includeHandler;
    includeHandler.loader = fileLoader;

    DXC::Target target = {};
    switch(stage)
    {
    case RHI::ShaderStage::VertexShader:   target = DXC::Target::Vulkan_1_3_VS_6_0; break;
    case RHI::ShaderStage::FragmentShader: target = DXC::Target::Vulkan_1_3_FS_6_0; break;
    case RHI::ShaderStage::ComputeShader:  target = DXC::Target::Vulkan_1_3_CS_6_0; break;
    }

    // load source

    std::string rawSource;
    if(!source.source.empty())
    {
        rawSource = source.source;
    }
    else if(!fileLoader->Load(source.filename, rawSource))
    {
        throw Exception("failed to load shader source file: " + source.filename);
    }

    // temporary source file

    std::string sourceFilename, tempFilename;
    if(needTempSourceFile)
    {
        std::string tempFilePrefix;
        if(!source.filename.empty())
        {
            tempFilePrefix = std::filesystem::path(source.filename).filename().string();
        }
        tempFilename = GenerateTemporalFilename(tempFilePrefix) + ".hlsl";
        {
            std::ofstream fout(tempFilename, std::ofstream::out | std::ofstream::trunc);
            if(!fout)
            {
                throw Exception("failed to create temporal source file");
            }
            fout << rawSource;
        }
        sourceFilename = tempFilename;
    }
    else if(!source.filename.empty())
    {
        sourceFilename = fileLoader->GetMappedSourceFilename(source.filename);
    }
    else
    {
        sourceFilename = "anonymous.hlsl";
    }
    RTRC_SCOPE_EXIT
    {
        if(needTempSourceFile)
        {
            std::filesystem::remove(tempFilename);
        }
    };

    // preprocess

    std::vector<std::string> includeDirs;
    includeDirs.push_back(rootDir_.string());
    if(!source.filename.empty())
    {
        includeDirs.push_back(
            absolute(std::filesystem::path(source.filename)).parent_path().lexically_normal().string());
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
    dxc.Compile(shaderInfo, target, debug, &includeHandler, &preprocessedSource);

    // parse binding groups

    std::vector<ShaderBindingGroupRewriter::ParsedBindingGroup> parsedBindingGroups;
    {
        ShaderBindingGroupRewriter bindingGroupParser(preprocessedSource);
        while(true)
        {
            ShaderBindingGroupRewriter::ParsedBindingGroup group;
            if(bindingGroupParser.RewriteNextBindingGroup(group))
            {
                parsedBindingGroups.push_back(std::move(group));
            }
            else
            {
                break;
            }
        }
        preprocessedSource = bindingGroupParser.GetFinalSource();
    }

    // parse bindings

    std::vector<ShaderBindingParser::ParsedBinding> parsedBindings;
    {
        ShaderBindingParser bindingParser(preprocessedSource);
        while(true)
        {
            ShaderBindingParser::ParsedBinding binding;
            if(bindingParser.FindNextBinding(binding))
            {
                parsedBindings.push_back(binding);
            }
            else
            {
                break;
            }
        }
        preprocessedSource = bindingParser.GetFinalSource();
    }

    // map binding name to (group, slot)

    struct BindingRecord
    {
        int groupIndex;
        int slotIndex;
        const ShaderBindingParser::ParsedBinding *parsedBinding;
    };

    std::map<std::string, BindingRecord> bindingNameToRecord;
    for(auto &&[groupIndex, group] : Enumerate(parsedBindingGroups))
    {
        for(auto &&[slotIndex, aliasedBindings] : Enumerate(group.bindings))
        {
            for(auto &binding : aliasedBindings)
            {
                if(bindingNameToRecord.find(binding.name) != bindingNameToRecord.end())
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
        [](const ShaderBindingParser::ParsedBinding &a, const ShaderBindingParser::ParsedBinding &b)
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
    const std::vector<unsigned char> compileData = dxc.Compile(shaderInfo, target, debug, &includeHandler, nullptr);

    return rhiDevice_->CreateShader(compileData.data(), compileData.size(), source.entry, stage);
}

RTRC_END