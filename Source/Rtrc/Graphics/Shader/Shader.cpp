#include <Rtrc/Graphics/Shader/ShaderCompiler.h>
#include <Rtrc/Utility/Unreachable.h>

RTRC_BEGIN

const RC<BindingLayout> &ShaderInfo::GetBindingLayout() const
{
    return bindingLayout_;
}

Span<ShaderIOVar> ShaderInfo::GetInputVariables() const
{
    return VSInput_;
}

Span<ShaderConstantBuffer> ShaderInfo::GetConstantBuffers() const
{
    return constantBuffers_;
}

int ShaderInfo::GetBindingGroupCount() const
{
    return static_cast<int>(bindingGroupLayouts_.size());
}

const std::string &ShaderInfo::GetBindingGroupNameByIndex(int index) const
{
    return bindingGroupNames_[index];
}

const RC<BindingGroupLayout> &ShaderInfo::GetBindingGroupLayoutByName(std::string_view name) const
{
    static const RC<BindingGroupLayout> nil;
    const auto it = nameToBindingGroupLayoutIndex_.find(name);
    return it != nameToBindingGroupLayoutIndex_.end() ? GetBindingGroupLayoutByIndex(it->second) : nil;
}

const RC<BindingGroupLayout> &ShaderInfo::GetBindingGroupLayoutByIndex(int index) const
{
    return bindingGroupLayouts_[index];
}

int ShaderInfo::GetBindingGroupIndexByName(std::string_view name) const
{
    const auto it = nameToBindingGroupLayoutIndex_.find(name);
    return it != nameToBindingGroupLayoutIndex_.end() ? it->second : -1;
}

const RC<BindingGroup> &ShaderInfo::GetBindingGroupForInlineSamplers() const
{
    return bindingGroupForInlineSamplers_;
}

int ShaderInfo::GetBindingGroupIndexForInlineSamplers() const
{
    return bindingGroupForInlineSamplers_ ? (static_cast<int>(bindingGroupLayouts_.size()) - 1) : -1;
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

RTRC_END
