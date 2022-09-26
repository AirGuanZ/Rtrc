#pragma once

#include <map>

#include <Rtrc/Graphics/Shader/ShaderBindingGroup.h>
#include <Rtrc/Graphics/Shader/ShaderReflection.h>

RTRC_BEGIN

class Shader : public Uncopyable
{
public:

    const RHI::RawShaderPtr &GetRawShader(RHI::ShaderStage stage) const;
    const RHI::BindingLayoutPtr &GetRHIBindingLayout() const;

    Span<ShaderIOVar> GetInputVariables() const;
    Span<ShaderConstantBuffer> GetConstantBuffers() const;

    int GetBindingGroupCount() const;
    const std::string &GetBindingGroupNameByIndex(int index) const;
    const RC<BindingGroupLayout> &GetBindingGroupLayoutByName(std::string_view name) const;
    const RC<BindingGroupLayout> &TryGetBindingGroupLayoutByName(std::string_view name) const;
    const RC<BindingGroupLayout> &GetBindingGroupLayoutByIndex(int index) const;
    int GetBindingGroupIndexByName(std::string_view name) const;
    int TryGetBindingGroupIndexByName(std::string_view name) const; // return -1 when not found

private:

    friend class ShaderCompiler;

    struct BindingLayout
    {
        RHI::BindingLayoutPtr rhiPtr;
    };

    RHI::RawShaderPtr VS_;
    RHI::RawShaderPtr FS_;
    RHI::RawShaderPtr CS_;

    std::vector<ShaderIOVar>          VSInput_;
    std::vector<ShaderConstantBuffer> constantBuffers_;

    std::map<std::string, int, std::less<>> nameToBindingGroupLayoutIndex_;
    std::vector<RC<BindingGroupLayout>>     bindingGroupLayouts_;
    std::vector<std::string>                bindingGroupNames_;
    RC<BindingLayout>                       bindingLayout_;
};

RTRC_END
