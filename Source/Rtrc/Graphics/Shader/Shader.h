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

    const RC<BindingGroupLayout> GetBindingGroupLayoutByName(std::string_view name) const;
    const RC<BindingGroupLayout> GetBindingGroupLayoutByIndex(int index) const;
    int GetBindingGroupIndexByName(std::string_view name) const;

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
    RC<BindingLayout>                       bindingLayout_;
};

RTRC_END
