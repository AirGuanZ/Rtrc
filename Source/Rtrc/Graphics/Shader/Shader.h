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

// Ray tracing shader groups
class ShaderGroupInfo
{
public:

    Span<RHI::RayTracingRayGenShaderGroup> GetRayGenShaderGroups() const;
    Span<RHI::RayTracingMissShaderGroup>   GetMissShaderGroups()   const;
    Span<RHI::RayTracingHitShaderGroup>    GetHitShaderGroups()    const;

private:

    friend class ShaderCompiler;

    std::vector<RHI::RayTracingRayGenShaderGroup> rayGenShaderGroups_;
    std::vector<RHI::RayTracingMissShaderGroup>   missShaderGroups_;
    std::vector<RHI::RayTracingHitShaderGroup>    hitShaderGroups_;
};

class ShaderInfo : public Uncopyable
{
public:

    enum class Category
    {
        Graphics,
        Compute,
        RayTracing
    };

    enum class BuiltinBindingGroup
    {
        Pass,
        Material,
        Object,
        Count
    };

    using PushConstantRange = RHI::PushConstantRange;

    virtual ~ShaderInfo() = default;

    Category GetCategory() const;

    const RC<BindingLayout> &GetBindingLayout() const;

    Span<ShaderIOVar> GetInputVariables() const;
    Span<ShaderConstantBuffer> GetConstantBuffers() const;

    // Binding group

    int GetBindingGroupCount() const;

    const std::string &GetBindingGroupNameByIndex(int index) const;
    int GetBindingGroupIndexByName(std::string_view name) const; // return -1 if not found

    const RC<BindingGroupLayout> &GetBindingGroupLayoutByName(std::string_view name) const; // returns -1 if not found
    const RC<BindingGroupLayout> &GetBindingGroupLayoutByIndex(int index) const;

    int GetBuiltinBindingGroupIndex(BuiltinBindingGroup bindingGroup) const;

    const ShaderBindingNameMap &GetBindingNameMap() const;

    // Inline samplers
    
    const RC<BindingGroup> &GetBindingGroupForInlineSamplers() const;
    int GetBindingGroupIndexForInlineSamplers() const;

    // Compute thread group

    const Vector3i &GetThreadGroupSize() const; // Compute shader only
    Vector3i ComputeThreadGroupCount(const Vector3i &threadCount) const;

    // Push constant

    Span<PushConstantRange> GetPushConstantRanges() const;

    // Ray tracing shader group

    const ShaderGroupInfo &GetShaderGroupInfo() const;

private:

    friend class ShaderCompiler;

    Category category_ = Category::Graphics;

    std::vector<ShaderIOVar>          VSInput_;
    std::vector<ShaderConstantBuffer> constantBuffers_;

    std::map<std::string, int, std::less<>> nameToBindingGroupLayoutIndex_;
    std::vector<RC<BindingGroupLayout>>     bindingGroupLayouts_;
    std::vector<std::string>                bindingGroupNames_;
    RC<BindingLayout>                       bindingLayout_;
    ShaderBindingNameMap                    bindingNameMap_;
    int                                     builtinBindingGroupIndices_[EnumCount<BuiltinBindingGroup>] = {};

    // Should be the last group, if present
    RC<BindingGroup> bindingGroupForInlineSamplers_;

    Vector3i computeShaderThreadGroupSize_;

    std::vector<PushConstantRange> pushConstantRanges_;

    RC<const ShaderGroupInfo> shaderGroupInfo_;
};

class Shader : public Uncopyable, public WithUniqueObjectID
{
public:

    using Category            = ShaderInfo::Category;
    using BuiltinBindingGroup = ShaderInfo::BuiltinBindingGroup;
    using PushConstantRange   = ShaderInfo::PushConstantRange;
    
    Category GetCategory() const;
    const RHI::RawShaderPtr &GetRawShader(RHI::ShaderType type) const;

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

    Span<ShaderInfo::PushConstantRange> GetPushConstantRanges() const;

    Span<RHI::RayTracingRayGenShaderGroup> GetRayGenShaderGroups() const;
    Span<RHI::RayTracingMissShaderGroup>   GetMissShaderGroups()   const;
    Span<RHI::RayTracingHitShaderGroup>    GetHitShaderGroups()    const;

private:
    
    friend class ShaderCompiler;

    static constexpr int VS_INDEX = 0;
    static constexpr int FS_INDEX = 1;
    static constexpr int CS_INDEX = 0;
    static constexpr int RT_INDEX = 0;

    RHI::RawShaderPtr rawShaders_[2];
    
    RC<ShaderInfo> info_;
    RC<ComputePipeline> computePipeline_;
};

inline Span<RHI::RayTracingRayGenShaderGroup> ShaderGroupInfo::GetRayGenShaderGroups() const
{
    return rayGenShaderGroups_;
}

inline Span<RHI::RayTracingMissShaderGroup> ShaderGroupInfo::GetMissShaderGroups() const
{
    return missShaderGroups_;
}

inline Span<RHI::RayTracingHitShaderGroup> ShaderGroupInfo::GetHitShaderGroups() const
{
    return hitShaderGroups_;
}

inline ShaderInfo::Category ShaderInfo::GetCategory() const
{
    return category_;
}

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
    return builtinBindingGroupIndices_[std::to_underlying(bindingGroup)];
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

inline Span<ShaderInfo::PushConstantRange> ShaderInfo::GetPushConstantRanges() const
{
    return pushConstantRanges_;
}

inline const ShaderGroupInfo &ShaderInfo::GetShaderGroupInfo() const
{
    assert(shaderGroupInfo_);
    return *shaderGroupInfo_;
}

inline const ShaderBindingNameMap &ShaderInfo::GetBindingNameMap() const
{
    return bindingNameMap_;
}

inline Shader::Category Shader::GetCategory() const
{
    return info_->GetCategory();
}

inline const RHI::RawShaderPtr &Shader::GetRawShader(RHI::ShaderType type) const
{
    using enum RHI::ShaderType;
    using enum Category;
    switch(type)
    {
    case VertexShader:     assert(GetCategory() == Graphics);   return rawShaders_[VS_INDEX];
    case FragmentShader:   assert(GetCategory() == Graphics);   return rawShaders_[FS_INDEX];
    case ComputeShader:    assert(GetCategory() == Compute);    return rawShaders_[CS_INDEX];
    case RayTracingShader: assert(GetCategory() == RayTracing); return rawShaders_[RT_INDEX];
    default:
        break;
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

inline Span<ShaderInfo::PushConstantRange> Shader::GetPushConstantRanges() const
{
    return info_->GetPushConstantRanges();
}

inline Span<RHI::RayTracingRayGenShaderGroup> Shader::GetRayGenShaderGroups() const
{
    return info_->GetShaderGroupInfo().GetRayGenShaderGroups();
}

inline Span<RHI::RayTracingMissShaderGroup> Shader::GetMissShaderGroups() const
{
    return info_->GetShaderGroupInfo().GetMissShaderGroups();
}

inline Span<RHI::RayTracingHitShaderGroup> Shader::GetHitShaderGroups() const
{
    return info_->GetShaderGroupInfo().GetHitShaderGroups();
}

RTRC_END
