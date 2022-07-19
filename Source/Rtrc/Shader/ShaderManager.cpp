#include <filesystem>
#include <iterator>

#include <fmt/format.h>

#include <Rtrc/Shader/DXC.h>
#include <Rtrc/Shader/ShaderBindingParser.h>
#include <Rtrc/Shader/ShaderManager.h>
#include <Rtrc/Utils/File.h>
#include <Rtrc/Utils/Unreachable.h>

RTRC_BEGIN

namespace
{

    class DefaultShaderFileLoader : public ShaderFileLoader
    {
    public:

        explicit DefaultShaderFileLoader(std::string_view rootDir)
            : rootDir_(absolute(std::filesystem::path(rootDir)).lexically_normal())
        {
            
        }

        bool Load(std::string_view filename, std::string &output) const override
        {
            std::filesystem::path path = filename;
            if(path.is_relative())
            {
                path = rootDir_ / path;
            }
            path = path.lexically_normal();
            try
            {
                output = File::ReadTextFile(path.string());
                return true;
            }
            catch(...)
            {
                return false;
            }
        }

    private:

        std::filesystem::path rootDir_;
    };

    class FakeShaderFileLoader : public ShaderFileLoader
    {
    public:

        bool Load(std::string_view filename, std::string &output) const override
        {
            return false;
        }
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

RC<BindingGroup> BindingGroupLayout::AllocateBindingGroup() const
{
    RHI::BindingGroupPtr rhiGroup = rhiLayout_->CreateBindingGroup();
    return MakeRC<BindingGroup>(this, std::move(rhiGroup));
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

ShaderManager::ShaderManager()
    : debug_(RTRC_DEBUG)
{
    
}

void ShaderManager::SetDebugMode(bool enableDebug)
{
    debug_ = enableDebug;
}

void ShaderManager::SetFileLoader(std::string_view rootDir)
{
    fileLoader_ = MakeRC<DefaultShaderFileLoader>(rootDir);
}

void ShaderManager::SetCustomFileLoader(RC<ShaderFileLoader> customLoader)
{
    fileLoader_ = std::move(customLoader);
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

    if(!fileLoader_)
    {
        fileLoader_ = MakeRC<FakeShaderFileLoader>();
    }
    RC<ShaderFileLoader> fileLoader = fileLoader_;
    if(desc.overrideFileLoader)
    {
        fileLoader = desc.overrideFileLoader;
    }

    bool debug = debug_;
    if(desc.overrideDebugMode.has_value())
    {
        debug = desc.overrideDebugMode.value();
    }

    std::map<std::string, int, std::less<>> nameToGroupIndex;
    std::vector<RC<BindingGroupLayout>> bindingGroupLayouts;

    RHI::RawShaderPtr VS, FS, CS;
    if(!desc.VS.filename.empty() || !desc.VS.source.empty())
    {
        VS = CompileShader(
            debug, fileLoader, macros, desc.VS, RHI::ShaderStage::VertexShader,
            nameToGroupIndex, bindingGroupLayouts);
    }
    if(!desc.FS.filename.empty() || !desc.FS.source.empty())
    {
        FS = CompileShader(
            debug, fileLoader, macros, desc.FS, RHI::ShaderStage::FragmentShader,
            nameToGroupIndex, bindingGroupLayouts);
    }
    if(!desc.CS.filename.empty() || !desc.CS.source.empty())
    {
        CS = CompileShader(
            debug, fileLoader, macros, desc.CS, RHI::ShaderStage::ComputeShader,
            nameToGroupIndex, bindingGroupLayouts);
    }

    RC<Shader> shader = MakeRC<Shader>();
    shader->VS_ = VS;
    shader->FS_ = FS;
    shader->CS_ = CS;
    shader->nameToBindingGroupLayoutIndex_ = std::move(nameToGroupIndex);
    shader->bindingGroupLayouts_ = std::move(bindingGroupLayouts);
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

RHI::RawShaderPtr ShaderManager::CompileShader(
    bool                                      debug,
    const RC<ShaderFileLoader>               &fileLoader,
    const std::map<std::string, std::string> &macros,
    const ShaderSource                       &source,
    RHI::ShaderStage                          stage,
    std::map<std::string, int, std::less<>>  &outputNameToGroupIndex,
    std::vector<RC<BindingGroupLayout>>      &outputBindingGroupLayouts)
{
    DXCIncludeHandlerWrapper includeHandler;
    includeHandler.loader = fileLoader;

    DXC::Target target = {};
    switch(stage)
    {
    case RHI::ShaderStage::VertexShader:   target = DXC::Target::Vulkan_1_3_VS_6_0; break;
    case RHI::ShaderStage::FragmentShader: target = DXC::Target::Vulkan_1_3_FS_6_0; break;
    case RHI::ShaderStage::ComputeShader:  target = DXC::Target::Vulkan_1_3_CS_6_0; break;
    }

    DXC dxc;

    // preprocess

    std::string rawSource;
    if(!source.source.empty())
    {
        rawSource = source.source;
    }
    else if(!fileLoader_->Load(source.filename, rawSource))
    {
        throw Exception("failed to load shader source file: " + source.filename);
    }

    std::string preprocessedSource;
    const DXC::ShaderInfo shaderInfo =
    {
        .source         = rawSource,
        .sourceFilename = source.filename.empty() ? "anonymous.hlsl" : source.filename,
        .entryPoint     = source.entry,
        .macros         = macros
    };
    dxc.Compile(shaderInfo, target, debug, &includeHandler, &preprocessedSource);

    // parse binding groups

    ShaderBindingParser parser(preprocessedSource);
    std::vector<ShaderBindingParser::ParsedBindingGroup> parsedBindingGroups;
    while(true)
    {
        ShaderBindingParser::ParsedBindingGroup group;
        if(parser.ProcessNextBindingGroup(group))
        {
            printf("parsed group: %s\n", group.name.c_str());
            for(auto &aliasedNames : group.bindings)
            {
                for(auto &n : aliasedNames)
                {
                    printf("%s ", n.c_str());
                }
                printf("\n");
            }
            parsedBindingGroups.push_back(std::move(group));
        }
        else
        {
            break;
        }
    }
    preprocessedSource = parser.GetFinalSource();

    //printf("%s\n", preprocessedSource.c_str());

    // TODO
    return {};
}

RTRC_END
