#include <Rtrc/Graphics/Shader/ShaderCompiler.h>
#include <Rtrc/Utility/Unreachable.h>

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

const RC<BindingLayout> &Shader::GetBindingLayout() const
{
    return bindingLayout_;
}

Span<ShaderIOVar> Shader::GetInputVariables() const
{
    return VSInput_;
}

Span<ShaderConstantBuffer> Shader::GetConstantBuffers() const
{
    return constantBuffers_;
}

int Shader::GetBindingGroupCount() const
{
    return static_cast<int>(bindingGroupLayouts_.size());
}

const std::string &Shader::GetBindingGroupNameByIndex(int index) const
{
    return bindingGroupNames_[index];
}

const RC<BindingGroupLayout> &Shader::GetBindingGroupLayoutByName(std::string_view name) const
{
    const int index = GetBindingGroupIndexByName(name);
    if(index < 0)
    {
        throw Exception(fmt::format("unknown binding group layout '{}' in shader", name));
    }
    return GetBindingGroupLayoutByIndex(index);
}

const RC<BindingGroupLayout> &Shader::TryGetBindingGroupLayoutByName(std::string_view name) const
{
    static const RC<BindingGroupLayout> nil;
    const auto it = nameToBindingGroupLayoutIndex_.find(name);
    return it != nameToBindingGroupLayoutIndex_.end() ? GetBindingGroupLayoutByIndex(it->second) : nil;
}

const RC<BindingGroupLayout> &Shader::GetBindingGroupLayoutByIndex(int index) const
{
    return bindingGroupLayouts_[index];
}

int Shader::GetBindingGroupIndexByName(std::string_view name) const
{
    const auto it = nameToBindingGroupLayoutIndex_.find(name);
    return it != nameToBindingGroupLayoutIndex_.end() ? it->second : -1;
}

const RC<BindingGroup> &Shader::GetBindingGroupForInlineSamplers() const
{
    return bindingGroupForInlineSamplers_;
}

int Shader::GetBindingGroupIndexForInlineSamplers() const
{
    return bindingGroupForInlineSamplers_ ? (static_cast<int>(bindingGroupLayouts_.size()) - 1) : -1;
}

RTRC_END
