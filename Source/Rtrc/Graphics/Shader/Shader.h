#pragma once

#include <map>

#include <Rtrc/Graphics/Shader/ParsedBindingGroup.h>
#include <Rtrc/Graphics/Shader/ShaderBindingNameMap.h>
#include <Rtrc/Graphics/Shader/ShaderReflection.h>
#include <Rtrc/Utility/Unreachable.h>

RTRC_BEGIN

class BindingLayout;
class BindingGroupLayout;
class ComputePipeline;
class ShaderCompiler;

class ShaderInfo : public Uncopyable
{
public:

    enum class BuiltinBindingGroup
    {
        Pass,
        Material,
        Object,
        Count
    };

    virtual ~ShaderInfo() = default;

    const RC<BindingLayout> &GetBindingLayout() const;

    Span<ShaderIOVar> GetInputVariables() const;
    Span<ShaderConstantBuffer> GetConstantBuffers() const;

    int GetBindingGroupCount() const;

    const std::string &GetBindingGroupNameByIndex(int index) const;
    int GetBindingGroupIndexByName(std::string_view name) const; // return -1 if not found

    const RC<BindingGroupLayout> &GetBindingGroupLayoutByName(std::string_view name) const; // returns -1 if not found
    const RC<BindingGroupLayout> &GetBindingGroupLayoutByIndex(int index) const;
    
    const RC<BindingGroup> &GetBindingGroupForInlineSamplers() const;
    int GetBindingGroupIndexForInlineSamplers() const;

    int GetBuiltinBindingGroupIndex(BuiltinBindingGroup bindingGroup) const;
    const ShaderBindingNameMap &GetBindingNameMap() const;

    const Vector3i &GetThreadGroupSize() const; // Compute shader only
    Vector3i ComputeThreadGroupCount(const Vector3i &threadCount) const;

private:

    friend class ShaderCompiler;

    std::vector<ShaderIOVar>          VSInput_;
    std::vector<ShaderConstantBuffer> constantBuffers_;

    std::map<std::string, int, std::less<>> nameToBindingGroupLayoutIndex_;
    std::vector<RC<BindingGroupLayout>>     bindingGroupLayouts_;
    std::vector<std::string>                bindingGroupNames_;
    RC<BindingLayout>                       bindingLayout_;

    // Should be the last group, if present
    RC<BindingGroup> bindingGroupForInlineSamplers_;

    int builtinBindingGroupIndices_[EnumCount<BuiltinBindingGroup>];

    Vector3i computeShaderThreadGroupSize_;

    ShaderBindingNameMap bindingNameMap_;
};

class Shader : public Uncopyable, public WithUniqueObjectID
{
public:

    using BuiltinBindingGroup = ShaderInfo::BuiltinBindingGroup;

    const RHI::RawShaderPtr &GetRawShader(RHI::ShaderStage stage) const;

    const RC<ShaderInfo> &GetInfo() const;

    // Wrapped interfaces of ShaderInfo

    const RC<BindingLayout> &GetBindingLayout() const;

    Span<ShaderIOVar> GetInputVariables() const;
    Span<ShaderConstantBuffer> GetConstantBuffers() const;

    int GetBindingGroupCount() const;

    const std::string &GetBindingGroupNameByIndex(int index) const;
    int GetBindingGroupIndexByName(std::string_view name) const; // returns -1 if not found

    const RC<BindingGroupLayout> &GetBindingGroupLayoutByName(std::string_view name) const;
    const RC<BindingGroupLayout> &GetBindingGroupLayoutByIndex(int index) const;

    const RC<BindingGroup> &GetBindingGroupForInlineSamplers() const;
    int GetBindingGroupIndexForInlineSamplers() const;

    int GetBuiltinBindingGroupIndex(ShaderInfo::BuiltinBindingGroup bindingGroup) const;
    int GetPerPassBindingGroup() const;
    int GetPerMaterialBindingGroup() const;
    int GetPerObjectBindingGroup() const;
    const ShaderBindingNameMap &GetBindingNameMap() const;

    const RC<ComputePipeline> &GetComputePipeline() const;
    const Vector3i &GetThreadGroupSize() const; // Compute shader only
    Vector3i ComputeThreadGroupCount(const Vector3i &threadCount) const;

private:
    
    friend class ShaderCompiler;

    RHI::RawShaderPtr VS_;
    RHI::RawShaderPtr FS_;
    RHI::RawShaderPtr CS_;
    RC<ShaderInfo> info_;
    RC<ComputePipeline> computePipeline_;
};

inline const RC<BindingLayout> &ShaderInfo::GetBindingLayout() const
{
    return bindingLayout_;
}

inline Span<ShaderIOVar> ShaderInfo::GetInputVariables() const
{
    return VSInput_;
}

inline Span<ShaderConstantBuffer> ShaderInfo::GetConstantBuffers() const
{
    return constantBuffers_;
}

inline int ShaderInfo::GetBindingGroupCount() const
{
    return static_cast<int>(bindingGroupLayouts_.size());
}

inline const std::string &ShaderInfo::GetBindingGroupNameByIndex(int index) const
{
    return bindingGroupNames_[index];
}

inline const RC<BindingGroupLayout> &ShaderInfo::GetBindingGroupLayoutByName(std::string_view name) const
{
    static const RC<BindingGroupLayout> nil;
    const auto it = nameToBindingGroupLayoutIndex_.find(name);
    return it != nameToBindingGroupLayoutIndex_.end() ? GetBindingGroupLayoutByIndex(it->second) : nil;
}

inline const RC<BindingGroupLayout> &ShaderInfo::GetBindingGroupLayoutByIndex(int index) const
{
    return bindingGroupLayouts_[index];
}

inline int ShaderInfo::GetBindingGroupIndexByName(std::string_view name) const
{
    const auto it = nameToBindingGroupLayoutIndex_.find(name);
    return it != nameToBindingGroupLayoutIndex_.end() ? it->second : -1;
}

inline const RC<BindingGroup> &ShaderInfo::GetBindingGroupForInlineSamplers() const
{
    return bindingGroupForInlineSamplers_;
}

inline int ShaderInfo::GetBindingGroupIndexForInlineSamplers() const
{
    return bindingGroupForInlineSamplers_ ? (static_cast<int>(bindingGroupLayouts_.size()) - 1) : -1;
}

inline int ShaderInfo::GetBuiltinBindingGroupIndex(BuiltinBindingGroup bindingGroup) const
{
    return builtinBindingGroupIndices_[EnumToInt(bindingGroup)];
}

inline const Vector3i &ShaderInfo::GetThreadGroupSize() const
{
    return computeShaderThreadGroupSize_;
}

inline Vector3i ShaderInfo::ComputeThreadGroupCount(const Vector3i &threadCount) const
{
    return {
        (threadCount.x + computeShaderThreadGroupSize_.x - 1) / computeShaderThreadGroupSize_.x,
        (threadCount.y + computeShaderThreadGroupSize_.y - 1) / computeShaderThreadGroupSize_.y,
        (threadCount.z + computeShaderThreadGroupSize_.z - 1) / computeShaderThreadGroupSize_.z
    };
}

inline const ShaderBindingNameMap &ShaderInfo::GetBindingNameMap() const
{
    return bindingNameMap_;
}

inline const RHI::RawShaderPtr &Shader::GetRawShader(RHI::ShaderStage stage) const
{
    switch(stage)
    {
    case RHI::ShaderStage::VertexShader:   return VS_;
    case RHI::ShaderStage::FragmentShader: return FS_;
    case RHI::ShaderStage::ComputeShader:  return CS_;
    }
    Unreachable();
}

inline const RC<ShaderInfo> &Shader::GetInfo() const
{
    return info_;
}

inline const RC<BindingLayout> &Shader::GetBindingLayout() const
{
    return info_->GetBindingLayout();
}

inline Span<ShaderIOVar> Shader::GetInputVariables() const
{
    return info_->GetInputVariables();
}

inline Span<ShaderConstantBuffer> Shader::GetConstantBuffers() const
{
    return info_->GetConstantBuffers();
}

inline int Shader::GetBindingGroupCount() const
{
    return info_->GetBindingGroupCount();
}

inline const std::string &Shader::GetBindingGroupNameByIndex(int index) const
{
    return info_->GetBindingGroupNameByIndex(index);
}

inline const RC<BindingGroupLayout> &Shader::GetBindingGroupLayoutByName(std::string_view name) const
{
    return info_->GetBindingGroupLayoutByName(name);
}

inline const RC<BindingGroupLayout> &Shader::GetBindingGroupLayoutByIndex(int index) const
{
    return info_->GetBindingGroupLayoutByIndex(index);
}

inline int Shader::GetBindingGroupIndexByName(std::string_view name) const
{
    return info_->GetBindingGroupIndexByName(name);
}

inline const RC<BindingGroup> &Shader::GetBindingGroupForInlineSamplers() const
{
    return info_->GetBindingGroupForInlineSamplers();
}

inline int Shader::GetBindingGroupIndexForInlineSamplers() const
{
    return info_->GetBindingGroupIndexForInlineSamplers();
}

inline int Shader::GetBuiltinBindingGroupIndex(ShaderInfo::BuiltinBindingGroup bindingGroup) const
{
    return info_->GetBuiltinBindingGroupIndex(bindingGroup);
}

inline int Shader::GetPerPassBindingGroup() const
{
    return GetBuiltinBindingGroupIndex(BuiltinBindingGroup::Pass);
}

inline int Shader::GetPerMaterialBindingGroup() const
{
    return GetBuiltinBindingGroupIndex(BuiltinBindingGroup::Material);
}

inline int Shader::GetPerObjectBindingGroup() const
{
    return GetBuiltinBindingGroupIndex(BuiltinBindingGroup::Object);
}

inline const ShaderBindingNameMap &Shader::GetBindingNameMap() const
{
    return info_->GetBindingNameMap();
}

inline const RC<ComputePipeline> &Shader::GetComputePipeline() const
{
    return computePipeline_;
}

inline const Vector3i &Shader::GetThreadGroupSize() const
{
    return info_->GetThreadGroupSize();
}

inline Vector3i Shader::ComputeThreadGroupCount(const Vector3i &threadCount) const
{
    return info_->ComputeThreadGroupCount(threadCount);
}

RTRC_END
