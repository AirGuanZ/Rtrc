#pragma once

#include <map>

#include <Rtrc/RHI/RHI.h>

RTRC_BEGIN

class BindingGroupLayout;
class ShaderManager;

class ParsedBindingGroupLayout : public Uncopyable
{
public:

    using ParsedBindingDescription = RHI::BindingDesc;

    void AppendSlot(Span<ParsedBindingDescription> aliasedDescs);

    int GetBindingSlotCount() const;
    Span<ParsedBindingDescription> GetBindingDescriptionsBySlot(int slot) const;

private:

    struct Record
    {
        uint32_t offset;
        uint32_t count;
    };

    std::vector<Record> slotRecords_;
    std::vector<ParsedBindingDescription> allDescs_;
};

class BindingGroup : public Uncopyable
{
public:

    BindingGroup(const BindingGroupLayout *parentLayout, RHI::BindingGroupPtr rhiGroup);

    void Set(int slot, const RHI::BufferPtr &cbuffer, size_t offset, size_t bytes);
    void Set(int slot, const RHI::SamplerPtr &sampler);
    void Set(int slot, const RHI::BufferSRVPtr &srv);
    void Set(int slot, const RHI::BufferUAVPtr &uav);
    void Set(int slot, const RHI::Texture2DSRVPtr &srv);
    void Set(int slot, const RHI::Texture2DUAVPtr &uav);

    void Set(std::string_view name, const RHI::BufferPtr &cbuffer, size_t offset, size_t bytes);
    void Set(std::string_view name, const RHI::SamplerPtr &sampler);
    void Set(std::string_view name, const RHI::BufferSRVPtr &srv);
    void Set(std::string_view name, const RHI::BufferUAVPtr &uav);
    void Set(std::string_view name, const RHI::Texture2DSRVPtr &srv);
    void Set(std::string_view name, const RHI::Texture2DUAVPtr &uav);

    RHI::BindingGroupPtr GetRHIBindingGroup();

private:

    const BindingGroupLayout *parentLayout_;
    RHI::BindingGroupPtr rhiGroup_;
};

class BindingGroupLayout : public Uncopyable
{
public:

    const std::string &GetGroupName() const;
    int GetBindingSlotByName(std::string_view bindingName) const;
    RHI::BindingGroupLayoutPtr GetRHIBindingGroupLayout();

    RC<BindingGroup> CreateBindingGroup() const;

private:

    friend class ShaderManager;

    std::string groupName_;
    std::map<std::string, int, std::less<>> bindingNameToSlot_;
    RHI::BindingGroupLayoutPtr rhiLayout_;
};

class ShaderFileLoader
{
public:

    virtual ~ShaderFileLoader() = default;

    virtual bool Load(std::string_view filename, std::string &output) const = 0;
};

class Shader : public Uncopyable
{
public:

    ~Shader();

    const RHI::RawShaderPtr &GetRawShader(RHI::ShaderStage stage) const;
    const RHI::BindingLayoutPtr &GetRHIBindingLayout() const;

    const RC<BindingGroupLayout> GetBindingGroupLayoutByName(std::string_view name) const;
    const RC<BindingGroupLayout> GetBindingGroupLayoutByIndex(int index) const;

    int GetBindingGroupIndexByName(std::string_view name) const;

private:

    friend class ShaderManager;

    struct BindingGroupLayoutRecord
    {
        RC<BindingGroupLayout> layout;
        int shaderCounter = 0; // a record is kept in parentManager_ until there is no shader referencing it

        BindingGroupLayoutRecord() = default;
        BindingGroupLayoutRecord(const BindingGroupLayoutRecord &other);
        BindingGroupLayoutRecord &operator=(const BindingGroupLayoutRecord &other);
    };

    struct BindingLayoutRecord
    {
        RHI::BindingLayoutPtr layout;
        int shaderCounter = 0;
    };

    using BindingGroupLayoutRecordIt = std::map<RHI::BindingGroupLayoutDesc, BindingGroupLayoutRecord>::iterator;
    using BindingLayoutRecordIt = std::map<RHI::BindingLayoutDesc, BindingLayoutRecord>::iterator;

    ShaderManager *parentManager_ = nullptr;

    RHI::RawShaderPtr VS_;
    RHI::RawShaderPtr FS_;
    RHI::RawShaderPtr CS_;

    std::map<std::string, int, std::less<>> nameToBindingGroupLayoutIndex_;
    std::vector<RC<BindingGroupLayout>>     bindingGroupLayouts_;
    std::vector<BindingGroupLayoutRecordIt> bindingGroupLayoutIterators_;
    BindingLayoutRecordIt                   bindingLayoutIterator_;
};

class ShaderManager : public Uncopyable
{
public:

    struct ShaderSource
    {
        std::string filename;
        std::string source;
        std::string entry;

        std::string *dumpedPreprocessedSource = nullptr;
    };

    struct ShaderDescription
    {
        ShaderSource VS;
        ShaderSource FS;
        ShaderSource CS;
        std::optional<bool>                overrideDebugMode;
        std::map<std::string, std::string> overrideMacros;
    };

    explicit ShaderManager(RHI::DevicePtr device = nullptr);

    void SetDevice(RHI::DevicePtr device);
    void SetDebugMode(bool enableDebug);
    void SetFileLoader(std::string_view rootDir);
    void AddMacro(std::string key, std::string value);

    RC<Shader> AddShader(const ShaderDescription &desc);

    const RC<BindingGroupLayout> &GetBindingGroupLayoutByName(std::string_view name) const;

    void OnShaderDestroyed(std::vector<Shader::BindingGroupLayoutRecordIt> &its, Shader::BindingLayoutRecordIt itt);

private:

    RHI::RawShaderPtr CompileShader(
        bool                                             debug,
        const std::map<std::string, std::string>        &macros,
        const ShaderSource                              &source,
        RHI::ShaderStage                                 stage,
        std::map<std::string, int, std::less<>>         &outputNameToGroupIndex,
        std::vector<RC<BindingGroupLayout>>             &bindingGroupLayouts,
        std::vector<Shader::BindingGroupLayoutRecordIt> &outputBindingGroupLayouts);

    RHI::DevicePtr rhiDevice_;

    bool debug_;
    std::filesystem::path rootDir_;
    std::map<std::string, std::string> macros_;

    // when value is nil, there is multiple binding groups sharing the same name
    std::map<std::string, RC<BindingGroupLayout>, std::less<>> nameToBindingGroupLayout_;

    std::map<RHI::BindingGroupLayoutDesc, Shader::BindingGroupLayoutRecord> descToBindingGroupLayout_;

    std::map<RHI::BindingLayoutDesc, Shader::BindingLayoutRecord> descToBindingLayout_;
};

RTRC_END