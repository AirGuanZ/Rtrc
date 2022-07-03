#pragma once

#include <map>

#include <Rtrc/RHI/RHI.h>
#include <Rtrc/Shader/Binding.h>
#include <Rtrc/Utils/TypeIndex.h>

RTRC_BEGIN

class Shader : public Uncopyable
{
public:

    Shader(RC<RHI::RawShader> vertexShader, RC<RHI::RawShader> fragmentShader, RC<RHI::RawShader> computeShader);

    const RC<RHI::RawShader> &GetVertexShader() const;

    const RC<RHI::RawShader> &GetFragmentShader() const;

    const RC<RHI::RawShader> &GetComputeShader() const;

private:

    RC<RHI::RawShader> vertexShader_;
    RC<RHI::RawShader> fragmentShader_;
    RC<RHI::RawShader> computeShader_;
};

class ShaderCompiler : public Uncopyable
{
public:

    enum class Target
    {
        Vulkan
    };

    ShaderCompiler &SetTarget(Target target);

    ShaderCompiler &SetDebugMode(bool debug);

    ShaderCompiler &SetVertexShaderSource(std::string source, std::string entry);

    ShaderCompiler &SetFragmentShaderSource(std::string source, std::string entry);

    ShaderCompiler &SetComputeShaderSource(std::string source, std::string entry);

    ShaderCompiler &AddMacro(std::string name, std::string value);

    ShaderCompiler &AddBindingGroup(const RHI::BindingGroupLayoutDesc *groupLayoutDesc);

    template<BindingGroupStruct BindingGroup>
    ShaderCompiler &AddBindingGroup() { return AddBindingGroup(GetBindingGroupLayoutDesc<BindingGroup>()); }

    RC<Shader> Compile(RHI::Device &device) const;

private:

    struct ShaderSource
    {
        std::string source;
        std::string entry;
    };

    RC<Shader> CompileHLSLForVulkan(RHI::Device &device) const;

    Target target_ = Target::Vulkan;
    bool debug_ = false;

    ShaderSource vertexShaderSource_;
    ShaderSource fragmentShaderSource_;
    ShaderSource computeShaderSource_;
    std::map<std::string, std::string> macros_;

    std::vector<const RHI::BindingGroupLayoutDesc *> bindingGroupLayoutDescs_;
};

RTRC_END
