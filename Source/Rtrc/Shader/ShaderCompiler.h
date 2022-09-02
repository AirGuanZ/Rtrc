#pragma once

#include <filesystem>
#include <map>

#include <Rtrc/Shader/ShaderBindingGroup.h>
#include <Rtrc/Shader/ShaderReflection.h>

RTRC_BEGIN

// TODO: 禁止custom file loader，用-I保证dxc能正确生成debug符号，还是先preprocess，再parse，再正式编译
// TODO: 用weak_ptr替代shaderCounter

class Shader : public Uncopyable
{
public:

    ~Shader();

    const RHI::RawShaderPtr &GetRawShader(RHI::ShaderStage stage) const;
    const RHI::BindingLayoutPtr &GetRHIBindingLayout() const;

    Span<ShaderIOVar> GetInputVariables() const;

    const RC<BindingGroupLayout> GetBindingGroupLayoutByName(std::string_view name) const;
    const RC<BindingGroupLayout> GetBindingGroupLayoutByIndex(int index) const;
    int GetBindingGroupIndexByName(std::string_view name) const;

private:

    friend class ShaderCompiler;

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

    ShaderCompiler *parentManager_ = nullptr;

    RHI::RawShaderPtr VS_;
    RHI::RawShaderPtr FS_;
    RHI::RawShaderPtr CS_;

    ShaderReflection VSRefl_;
    ShaderReflection FSRefl_;
    ShaderReflection CSRefl_;

    std::map<std::string, int, std::less<>> nameToBindingGroupLayoutIndex_;
    std::vector<RC<BindingGroupLayout>>     bindingGroupLayouts_;
    std::vector<BindingGroupLayoutRecordIt> bindingGroupLayoutIterators_;
    BindingLayoutRecordIt                   bindingLayoutIterator_;
};

class ShaderCompiler : public Uncopyable
{
public:

    class FileLoader
    {
    public:

        virtual ~FileLoader() = default;

        virtual bool Load(std::string_view filename, std::string& output) const = 0;
    };

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
        std::optional<bool> overrideDebugMode;
        std::map<std::string, std::string> macros;
    };

    explicit ShaderCompiler(RHI::DevicePtr device = nullptr);

    void SetDevice(RHI::DevicePtr device);
    void SetDebugMode(bool enableDebug);
    void SetFileLoader(std::string_view rootDir);

    RC<Shader> Compile(const ShaderDescription &desc);

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
        std::vector<Shader::BindingGroupLayoutRecordIt> &outputBindingGroupLayouts,
        ShaderReflection                                &outputRefl);

    RHI::DevicePtr rhiDevice_;

    bool debug_;
    std::filesystem::path rootDir_;

    // when value is nil, there is multiple binding groups sharing the same name
    std::map<std::string, RC<BindingGroupLayout>, std::less<>> nameToBindingGroupLayout_;

    std::map<RHI::BindingGroupLayoutDesc, Shader::BindingGroupLayoutRecord> descToBindingGroupLayout_;
    std::map<RHI::BindingLayoutDesc, Shader::BindingLayoutRecord>           descToBindingLayout_;
};

RTRC_END
