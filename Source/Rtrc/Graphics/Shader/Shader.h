#pragma once

#include <map>

#include <Rtrc/Core/Container/Cache/SharedObjectCache.h>
#include <Rtrc/Core/SignalSlot.h>
#include <Rtrc/Core/Unreachable.h>
#include <Rtrc/ShaderCommon/Preprocess/ShaderPreprocessing.h>
#include <Rtrc/ShaderCommon/Reflection/ShaderReflection.h>

RTRC_BEGIN

class BindingLayout;
class BindingGroup;
class BindingGroupLayout;
class ShaderCompiler;
class ComputePipeline;
class ShaderBuilder;
class ShaderCompiler;

using ShaderPropertyName = GeneralPooledString;
#define RTRC_SHADER_PROPERTY_NAME(X) RTRC_GENERAL_POOLED_STRING(X)

// Ray tracing shader groups
class ShaderGroupInfo
{
public:

    Span<RHI::RayTracingRayGenShaderGroup> GetRayGenShaderGroups() const;
    Span<RHI::RayTracingMissShaderGroup>   GetMissShaderGroups()   const;
    Span<RHI::RayTracingHitShaderGroup>    GetHitShaderGroups()    const;

private:

    friend class ShaderCompiler;
    friend class ShaderBuilder;

    std::vector<RHI::RayTracingRayGenShaderGroup> rayGenShaderGroups_;
    std::vector<RHI::RayTracingMissShaderGroup>   missShaderGroups_;
    std::vector<RHI::RayTracingHitShaderGroup>    hitShaderGroups_;
};

class ShaderBindingLayoutInfo : public Uncopyable
{
public:

    enum class BuiltinBindingGroup
    {
        Pass,
        Material,
        Object,
        BindlessTexture,
        BindlessGeometryBuffer,
        Count
    };

    using PushConstantRange = RHI::PushConstantRange;

    // Binding Layout

    const RC<BindingLayout> &GetBindingLayout() const;

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

    // Push constant

    Span<PushConstantRange> GetPushConstantRanges() const;

private:

    friend class ShaderCompiler;
    friend class ShaderBuilder;

    std::map<std::string, int, std::less<>> nameToBindingGroupLayoutIndex_;
    std::vector<RC<BindingGroupLayout>>     bindingGroupLayouts_;
    std::vector<std::string>                bindingGroupNames_;
    RC<BindingLayout>                       bindingLayout_;
    ShaderBindingNameMap                    bindingNameMap_;
    int                                     builtinBindingGroupIndices_[EnumCount<BuiltinBindingGroup>] = {};

    // Should be the last group, if present
    RC<BindingGroup> bindingGroupForInlineSamplers_;
    std::vector<PushConstantRange> pushConstantRanges_;
};

class ShaderInfo : public Uncopyable
{
public:

    using BuiltinBindingGroup = ShaderBindingLayoutInfo::BuiltinBindingGroup;
    using PushConstantRange = ShaderBindingLayoutInfo::PushConstantRange;

    virtual ~ShaderInfo() = default;

    const RC<ShaderBindingLayoutInfo> &GetBindingLayoutInfo() const;

    ShaderCategory GetCategory() const;

    const RC<BindingLayout> &GetBindingLayout() const;

    Span<ShaderIOVar> GetInputVariables() const;
    
    // Binding group

    int GetBindingGroupCount() const;

    const std::string &GetBindingGroupNameByIndex(int index) const;
    int GetBindingGroupIndexByName(std::string_view name) const; // return -1 if not found

    const RC<BindingGroupLayout> &GetBindingGroupLayoutByName(std::string_view name) const; // returns -1 if not found
    const RC<BindingGroupLayout> &GetBindingGroupLayoutByIndex(int index) const;
    const RC<BindingGroupLayout> &GetBuiltinBindingGroupLayout(BuiltinBindingGroup bindingGroup) const;

    int GetBuiltinBindingGroupIndex(BuiltinBindingGroup bindingGroup) const;

    const ShaderBindingNameMap &GetBindingNameMap() const;

    // Inline samplers
    
    const RC<BindingGroup> &GetBindingGroupForInlineSamplers() const;
    int GetBindingGroupIndexForInlineSamplers() const;

    // Compute thread group

    const Vector3u &GetThreadGroupSize() const; // Compute shader only
    Vector3u ComputeThreadGroupCount(const Vector3u &threadCount) const;

    // Push constant

    Span<PushConstantRange> GetPushConstantRanges() const;

    // Ray tracing shader group

    const ShaderGroupInfo &GetShaderGroupInfo() const;

private:
    
    friend class ShaderCompiler;
    friend class ShaderBuilder;

    ShaderCategory category_ = ShaderCategory::ClassicalGraphics;

    std::vector<ShaderIOVar> VSInput_;

    Vector3u                    computeShaderThreadGroupSize_;
    RC<ShaderGroupInfo>         shaderGroupInfo_;
    RC<ShaderBindingLayoutInfo> shaderBindingLayoutInfo_;
};

class Shader : public WithUniqueObjectID, public InSharedObjectCache
{
public:

    using BuiltinBindingGroup = ShaderInfo::BuiltinBindingGroup;
    using PushConstantRange   = ShaderInfo::PushConstantRange;

    ~Shader();
    
    ShaderCategory GetCategory() const;
    RHI::RawShaderOPtr GetRawShader(RHI::ShaderType type) const;

    const RC<ShaderInfo> &GetInfo() const;

    // Wrapped interfaces of ShaderInfo

    const RC<BindingLayout> &GetBindingLayout() const;

    Span<ShaderIOVar> GetInputVariables() const;
    
    int GetBindingGroupCount() const;

    const std::string &GetBindingGroupNameByIndex(int index) const;
    int GetBindingGroupIndexByName(std::string_view name) const; // returns -1 if not found

    const RC<BindingGroupLayout> &GetBindingGroupLayoutByName(std::string_view name) const;
    const RC<BindingGroupLayout> &GetBindingGroupLayoutByIndex(int index) const;
    const RC<BindingGroupLayout> &GetBuiltinBindingGroupLayout(BuiltinBindingGroup bindingGroup) const;

    const RC<BindingGroup> &GetBindingGroupForInlineSamplers() const;
    int GetBindingGroupIndexForInlineSamplers() const;

    int GetBuiltinBindingGroupIndex(ShaderInfo::BuiltinBindingGroup bindingGroup) const;
    int GetPerPassBindingGroup() const;
    int GetPerMaterialBindingGroup() const;
    int GetPerObjectBindingGroup() const;
    const ShaderBindingNameMap &GetBindingNameMap() const;

    const RC<ComputePipeline> &GetComputePipeline() const;
    const Vector3u &GetThreadGroupSize() const; // Compute shader only
    Vector3u ComputeThreadGroupCount(const Vector3u &threadCount) const;
    
    Span<ShaderInfo::PushConstantRange> GetPushConstantRanges() const;

    Span<RHI::RayTracingRayGenShaderGroup> GetRayGenShaderGroups() const;
    Span<RHI::RayTracingMissShaderGroup>   GetMissShaderGroups()   const;
    Span<RHI::RayTracingHitShaderGroup>    GetHitShaderGroups()    const;

    template<typename...Args>
    Connection OnDestroy(Args&&...args) const { return onDestroyCallbacks_.Connect(std::forward<Args>(args)...); }

private:
    
    friend class ShaderCompiler;
    friend class ShaderBuilder;

    static constexpr int VS_INDEX = 0;
    static constexpr int FS_INDEX = 1;
    static constexpr int CS_INDEX = 0;
    static constexpr int RT_INDEX = 0;
    static constexpr int MS_INDEX = 0;
    static constexpr int TS_INDEX = 2;
    static constexpr int WG_INDEX = 0;

    RHI::RawShaderUPtr rawShaders_[3];
    
    RC<ShaderInfo>      info_;
    RC<ComputePipeline> computePipeline_;

    mutable Signal<SignalThreadPolicy::SpinLock> onDestroyCallbacks_;
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

inline const RC<BindingLayout> &ShaderBindingLayoutInfo::GetBindingLayout() const
{
    return bindingLayout_;
}

inline int ShaderBindingLayoutInfo::GetBindingGroupCount() const
{
    return static_cast<int>(bindingGroupLayouts_.size());
}

inline const std::string &ShaderBindingLayoutInfo::GetBindingGroupNameByIndex(int index) const
{
    return bindingGroupNames_[index];
}

inline const RC<BindingGroupLayout> &ShaderBindingLayoutInfo::GetBindingGroupLayoutByName(std::string_view name) const
{
    static const RC<BindingGroupLayout> nil;
    const auto it = nameToBindingGroupLayoutIndex_.find(name);
    return it != nameToBindingGroupLayoutIndex_.end() ? GetBindingGroupLayoutByIndex(it->second) : nil;
}

inline const RC<BindingGroupLayout> &ShaderBindingLayoutInfo::GetBindingGroupLayoutByIndex(int index) const
{
    return bindingGroupLayouts_[index];
}

inline int ShaderBindingLayoutInfo::GetBindingGroupIndexByName(std::string_view name) const
{
    const auto it = nameToBindingGroupLayoutIndex_.find(name);
    return it != nameToBindingGroupLayoutIndex_.end() ? it->second : -1;
}

inline const RC<BindingGroup> &ShaderBindingLayoutInfo::GetBindingGroupForInlineSamplers() const
{
    return bindingGroupForInlineSamplers_;
}

inline int ShaderBindingLayoutInfo::GetBindingGroupIndexForInlineSamplers() const
{
    return bindingGroupForInlineSamplers_ ? (static_cast<int>(bindingGroupLayouts_.size()) - 1) : -1;
}

inline int ShaderBindingLayoutInfo::GetBuiltinBindingGroupIndex(BuiltinBindingGroup bindingGroup) const
{
    return builtinBindingGroupIndices_[std::to_underlying(bindingGroup)];
}

inline Span<ShaderBindingLayoutInfo::PushConstantRange> ShaderBindingLayoutInfo::GetPushConstantRanges() const
{
    return pushConstantRanges_;
}

inline const ShaderBindingNameMap &ShaderBindingLayoutInfo::GetBindingNameMap() const
{
    return bindingNameMap_;
}

inline const RC<BindingLayout> &ShaderInfo::GetBindingLayout() const
{
    return shaderBindingLayoutInfo_->GetBindingLayout();
}

inline const RC<ShaderBindingLayoutInfo> &ShaderInfo::GetBindingLayoutInfo() const
{
    return shaderBindingLayoutInfo_;
}

inline ShaderCategory ShaderInfo::GetCategory() const
{
    return category_;
}

inline Span<ShaderIOVar> ShaderInfo::GetInputVariables() const
{
    return VSInput_;
}

inline int ShaderInfo::GetBindingGroupCount() const
{
    return shaderBindingLayoutInfo_->GetBindingGroupCount();
}

inline const std::string &ShaderInfo::GetBindingGroupNameByIndex(int index) const
{
    return shaderBindingLayoutInfo_->GetBindingGroupNameByIndex(index);
}

inline const RC<BindingGroupLayout> &ShaderInfo::GetBindingGroupLayoutByName(std::string_view name) const
{
    return shaderBindingLayoutInfo_->GetBindingGroupLayoutByName(name);
}

inline const RC<BindingGroupLayout> &ShaderInfo::GetBindingGroupLayoutByIndex(int index) const
{
    return shaderBindingLayoutInfo_->GetBindingGroupLayoutByIndex(index);
}

inline const RC<BindingGroupLayout> &ShaderInfo::GetBuiltinBindingGroupLayout(BuiltinBindingGroup bindingGroup) const
{
    if(auto index = GetBuiltinBindingGroupIndex(bindingGroup); index >= 0)
    {
        return GetBindingGroupLayoutByIndex(index);
    }
    static const RC<BindingGroupLayout> ret;
    return ret;
}

inline int ShaderInfo::GetBindingGroupIndexByName(std::string_view name) const
{
    return shaderBindingLayoutInfo_->GetBindingGroupIndexByName(name);
}

inline const RC<BindingGroup> &ShaderInfo::GetBindingGroupForInlineSamplers() const
{
    return shaderBindingLayoutInfo_->GetBindingGroupForInlineSamplers();
}

inline int ShaderInfo::GetBindingGroupIndexForInlineSamplers() const
{
    return shaderBindingLayoutInfo_->GetBindingGroupIndexForInlineSamplers();
}

inline int ShaderInfo::GetBuiltinBindingGroupIndex(BuiltinBindingGroup bindingGroup) const
{
    return shaderBindingLayoutInfo_->GetBuiltinBindingGroupIndex(bindingGroup);
}

inline const Vector3u &ShaderInfo::GetThreadGroupSize() const
{
    return computeShaderThreadGroupSize_;
}

inline Vector3u ShaderInfo::ComputeThreadGroupCount(const Vector3u &threadCount) const
{
    return {
        (threadCount.x + computeShaderThreadGroupSize_.x - 1) / computeShaderThreadGroupSize_.x,
        (threadCount.y + computeShaderThreadGroupSize_.y - 1) / computeShaderThreadGroupSize_.y,
        (threadCount.z + computeShaderThreadGroupSize_.z - 1) / computeShaderThreadGroupSize_.z
    };
}

inline Span<ShaderInfo::PushConstantRange> ShaderInfo::GetPushConstantRanges() const
{
    return shaderBindingLayoutInfo_->GetPushConstantRanges();
}

inline const ShaderGroupInfo &ShaderInfo::GetShaderGroupInfo() const
{
    assert(shaderGroupInfo_);
    return *shaderGroupInfo_;
}

inline const ShaderBindingNameMap &ShaderInfo::GetBindingNameMap() const
{
    return shaderBindingLayoutInfo_->GetBindingNameMap();
}

inline Shader::~Shader()
{
    onDestroyCallbacks_.Emit();
}

inline ShaderCategory Shader::GetCategory() const
{
    return info_->GetCategory();
}

inline RHI::RawShaderOPtr Shader::GetRawShader(RHI::ShaderType type) const
{
    using enum RHI::ShaderType;
    using enum ShaderCategory;
    switch(type)
    {
    case VertexShader:
        assert(GetCategory() == ClassicalGraphics);
        return rawShaders_[VS_INDEX];
    case FragmentShader:
        assert(GetCategory() == ClassicalGraphics || GetCategory() == MeshGraphics);
        return rawShaders_[FS_INDEX];
    case ComputeShader:
        assert(GetCategory() == Compute);
        return rawShaders_[CS_INDEX];
    case RayTracingShader:
        assert(GetCategory() == RayTracing);
        return rawShaders_[RT_INDEX];
    case TaskShader:
        assert(GetCategory() == MeshGraphics);
        return rawShaders_[TS_INDEX];
    case MeshShader:
        assert(GetCategory() == MeshGraphics);
        return rawShaders_[MS_INDEX];
    case WorkGraphShader:
        assert(GetCategory() == WorkGraph);
        return rawShaders_[WG_INDEX];
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

inline const RC<BindingGroupLayout> &Shader::GetBuiltinBindingGroupLayout(BuiltinBindingGroup bindingGroup) const
{
    return info_->GetBuiltinBindingGroupLayout(bindingGroup);
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

inline const Vector3u &Shader::GetThreadGroupSize() const
{
    return info_->GetThreadGroupSize();
}

inline Vector3u Shader::ComputeThreadGroupCount(const Vector3u &threadCount) const
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
