#pragma once

#include <Core/Math/Vector3.h>
#include <Core/Container/ObjectCache.h>
#include <Core/Container/SharedObjectPool.h>
#include <Core/SignalSlot.h>
#include <Core/StringPool.h>
#include <Graphics/Device/Device.h>
#include <Graphics/Shader/Keyword.h>
#include <Graphics/Shader/ShaderDatabase.h>

/* LegacyMaterial
    LegacyMaterial:
        PropertyLayout
        LegacyMaterialPass:
            ShaderTemplate
            PropertyReferenceLayout
    LegacyMaterialInstance:
        LegacyMaterial
        PropertySheet
        LegacyMaterialPassInstance:
            LegacyMaterialPass
            ConstantBuffer
            TempBindingGroup
*/

RTRC_BEGIN

class LegacyMaterialInstance;

using MaterialPassTag = GeneralPooledString;
#define RTRC_MATERIAL_PASS_TAG(X) RTRC_GENERAL_POOLED_STRING(X)

using FastKeywordSet      = FastKeywordSet;
using FastKeywordSetValue = FastKeywordSetValue;
using FastKeywordContext  = FastKeywordContext;

// Property declared at material scope
struct MaterialProperty
{
    enum class Type
    {
        // Uniform

        Float,
        Float2,
        Float3,
        Float4,
        UInt,
        UInt2,
        UInt3,
        UInt4,
        Int,
        Int2,
        Int3,
        Int4,

        // Resource

        Buffer,
        Texture,
        Sampler,
        AccelerationStructure,
    };

    static constexpr bool IsValue(Type type);
    bool IsValue() const;

    size_t GetValueSize() const;

    static const char *GetTypeName(Type type);
    const char        *GetTypeName() const;

    Type               type;
    ShaderPropertyName name;
};

// Describe how properties are stored in a material instance
class MaterialPropertyHostLayout
{
public:

    explicit MaterialPropertyHostLayout(std::vector<MaterialProperty> properties);

    Span<MaterialProperty> GetProperties() const;
    Span<MaterialProperty> GetResourceProperties() const;
    Span<MaterialProperty> GetValueProperties() const;

    size_t GetPropertyCount() const;
    size_t GetResourcePropertyCount() const;
    size_t GetValuePropertyCount() const;

    size_t GetValueBufferSize() const;
    size_t GetValueOffset(int index) const;
    size_t GetValueOffset(ShaderPropertyName name) const;

    // return -1 when not found
    int GetPropertyIndexByName(ShaderPropertyName name) const;
    const MaterialProperty &GetPropertyByName(ShaderPropertyName name) const;

private:

    // value0, value1, ..., resource0, resource1, ...
    std::vector<MaterialProperty>                    sortedProperties_;
    std::map<ShaderPropertyName, int, std::less<>> nameToIndex_;
    size_t                                           valueBufferSize_;
    std::vector<size_t>                              valueIndexToOffset_;
};

// Describe how to create constantBuffer/bindingGroup for a material pass instance
class LegacyMaterialPassPropertyLayout
{
public:

    // Uniform values in 'Material' binding group
    struct ValueReference
    {
        size_t offsetInValueBuffer;
        size_t offsetInConstantBuffer;
        size_t sizeInConstantBuffer;
    };

    // Resources in 'Material' binding group
    struct ResourceReference
    {
        MaterialProperty::Type type;
        size_t                 indexInProperties;
        size_t                 indexInBindingGroup;
    };

    using MaterialResource = Variant<BufferSrv, TextureSrv, RC<Texture>, RC<Sampler>, RC<Tlas>>;

    LegacyMaterialPassPropertyLayout(const MaterialPropertyHostLayout &materialPropertyLayout, const Shader &shader);

    int GetBindingGroupIndex() const;

    size_t GetConstantBufferSize() const;
    void FillConstantBufferContent(const void *valueBuffer, void *outputData) const;

    const RC<BindingGroupLayout> &GetBindingGroupLayout() const;
    void FillBindingGroup(
        BindingGroup          &bindingGroup,
        Span<MaterialResource> materialResources,
        RC<DynamicBuffer>      cbuffer) const;

private:

    size_t                      constantBufferSize_;
    int                         constantBufferIndexInBindingGroup_;
    std::vector<ValueReference> valueReferences_;

    int                            bindingGroupIndex_;
    RC<BindingGroupLayout>         bindingGroupLayout_;
    std::vector<ResourceReference> resourceReferences_;
};

class LegacyMaterialPass : public Uncopyable, public WithUniqueObjectID
{
public:
    
    const std::vector<std::string>     &GetTags() const;
    const std::vector<MaterialPassTag> &GetPooledTags() const;

    FastKeywordSetValue ExtractKeywordValueMask(const FastKeywordContext &keywordValues) const;

    RC<Shader> GetShader(FastKeywordSetValue mask = {});
    RC<Shader> GetShader(const FastKeywordContext &keywordValues);

    const LegacyMaterialPassPropertyLayout *GetPropertyLayout(FastKeywordSetValue mask);
    const LegacyMaterialPassPropertyLayout *GetPropertyLayout(const FastKeywordContext &keywordValues);

    const RC<ShaderTemplate> &GetShaderTemplate() const;

private:

    friend class LegacyMaterialManager;

    std::vector<std::string>     tags_;
    std::vector<MaterialPassTag> pooledTags_;

    RC<ShaderTemplate> shaderTemplate_;

    const MaterialPropertyHostLayout            *hostMaterialPropertyLayout_ = nullptr;
    tbb::spin_rw_mutex                           propertyLayoutsMutex_;
    std::vector<Box<LegacyMaterialPassPropertyLayout>> materialPassPropertyLayouts_;
};

class LegacyMaterial : public std::enable_shared_from_this<LegacyMaterial>, public InObjectCache, public WithUniqueObjectID
{
public:

    const std::string &GetName() const;

    Span<MaterialProperty>                GetProperties() const;
    const RC<MaterialPropertyHostLayout> &GetPropertyLayout() const;

    // return -1 when not found
    int GetPassIndexByTag(MaterialPassTag tag) const;

    RC<LegacyMaterialPass> GetPassByIndex(int index);
    RC<LegacyMaterialPass> GetPassByTag(MaterialPassTag tag);
    
    Span<RC<LegacyMaterialPass>> GetPasses() const;

    RC<LegacyMaterialInstance> CreateInstance() const;

private:

    friend class LegacyMaterialManager;

    Device *device_ = nullptr;

    std::string                                 name_;
    std::vector<RC<LegacyMaterialPass>>               passes_;
    std::map<MaterialPassTag, int, std::less<>> tagToIndex_;
    RC<MaterialPropertyHostLayout>              propertyLayout_;
};

constexpr bool MaterialProperty::IsValue(Type type)
{
    return static_cast<int>(type) < static_cast<int>(Type::Buffer);
}

inline bool MaterialProperty::IsValue() const
{
    return IsValue(type);
}

inline size_t MaterialProperty::GetValueSize() const
{
    static constexpr size_t sizes[] =
    {
        sizeof(float),    sizeof(Vector2f), sizeof(Vector3f), sizeof(Vector4f),
        sizeof(uint32_t), sizeof(Vector2u), sizeof(Vector3u), sizeof(Vector4u),
        sizeof(int32_t),  sizeof(Vector2i), sizeof(Vector3i), sizeof(Vector4i),
    };
    assert(static_cast<size_t>(type) < GetArraySize(sizes));
    return sizes[static_cast<int>(type)];
}

inline const char *MaterialProperty::GetTypeName(Type type)
{
    static const char *names[] =
    {
        "float", "float2", "float3", "float4",
        "uint",  "uint2",  "uint3",  "uint4",
        "int",   "int2",   "int3",   "int4",
        "Buffer", "Texture2D", "Sampler"
    };
    assert(static_cast<size_t>(type) < GetArraySize(names));
    return names[static_cast<int>(type)];
}

inline const char *MaterialProperty::GetTypeName() const
{
    return GetTypeName(type);
}

inline Span<MaterialProperty> MaterialPropertyHostLayout::GetProperties() const
{
    return sortedProperties_;
}

inline Span<MaterialProperty> MaterialPropertyHostLayout::GetResourceProperties() const
{
    const MaterialProperty *begin = sortedProperties_.data() + GetValuePropertyCount();
    const size_t count = sortedProperties_.size() - GetValuePropertyCount();
    return Span(begin, count);
}

inline Span<MaterialProperty> MaterialPropertyHostLayout::GetValueProperties() const
{
    return Span(sortedProperties_.data(), GetValuePropertyCount());
}

inline size_t MaterialPropertyHostLayout::GetPropertyCount() const
{
    return sortedProperties_.size();
}

inline size_t MaterialPropertyHostLayout::GetResourcePropertyCount() const
{
    return sortedProperties_.size() - GetValuePropertyCount();
}

inline size_t MaterialPropertyHostLayout::GetValuePropertyCount() const
{
    return valueIndexToOffset_.size();
}

inline size_t MaterialPropertyHostLayout::GetValueBufferSize() const
{
    return valueBufferSize_;
}

inline size_t MaterialPropertyHostLayout::GetValueOffset(int index) const
{
    return valueIndexToOffset_[index];
}

inline size_t MaterialPropertyHostLayout::GetValueOffset(ShaderPropertyName name) const
{
    auto it = nameToIndex_.find(name);
    if(it == nameToIndex_.end())
    {
        throw Exception(fmt::format("Property name not found: {}", name));
    }
    return GetValueOffset(it->second);
}

// return -1 when not found
inline int MaterialPropertyHostLayout::GetPropertyIndexByName(ShaderPropertyName name) const
{
    auto it = nameToIndex_.find(name);
    return it != nameToIndex_.end() ? it->second : -1;
}

inline const MaterialProperty &MaterialPropertyHostLayout::GetPropertyByName(ShaderPropertyName name) const
{
    const int index = GetPropertyIndexByName(name);
    if(index < 0)
    {
        throw Exception(fmt::format("Unknown material property: {}", name));
    }
    return GetProperties()[index];
}

inline int LegacyMaterialPassPropertyLayout::GetBindingGroupIndex() const
{
    return bindingGroupIndex_;
}

inline size_t LegacyMaterialPassPropertyLayout::GetConstantBufferSize() const
{
    return constantBufferSize_;
}

inline const RC<BindingGroupLayout> &LegacyMaterialPassPropertyLayout::GetBindingGroupLayout() const
{
    return bindingGroupLayout_;
}

inline const std::vector<std::string> &LegacyMaterialPass::GetTags() const
{
    return tags_;
}

inline const std::vector<MaterialPassTag> &LegacyMaterialPass::GetPooledTags() const
{
    return pooledTags_;
}

inline FastKeywordSetValue LegacyMaterialPass::ExtractKeywordValueMask(const FastKeywordContext &keywordValues) const
{
    return shaderTemplate_->GetKeywordSet().ExtractValue(keywordValues);
}

inline RC<Shader> LegacyMaterialPass::GetShader(FastKeywordSetValue mask)
{
    return shaderTemplate_->GetVariant(mask, true);
}

inline RC<Shader> LegacyMaterialPass::GetShader(const FastKeywordContext &keywordValues)
{
    return GetShader(shaderTemplate_->GetKeywordSet().ExtractValue(keywordValues));
}

inline const LegacyMaterialPassPropertyLayout *LegacyMaterialPass::GetPropertyLayout(FastKeywordSetValue mask)
{
    {
        std::shared_lock readLock(propertyLayoutsMutex_);
        if(materialPassPropertyLayouts_[mask.GetInternalValue()])
        {
            return materialPassPropertyLayouts_[mask.GetInternalValue()].get();
        }
    }

    std::unique_lock writeLock(propertyLayoutsMutex_);
    if(!materialPassPropertyLayouts_[mask.GetInternalValue()])
    {
        auto newLayout = MakeBox<LegacyMaterialPassPropertyLayout>(*hostMaterialPropertyLayout_, *GetShader(mask));
        materialPassPropertyLayouts_[mask.GetInternalValue()] = std::move(newLayout);
    }
    return materialPassPropertyLayouts_[mask.GetInternalValue()].get();
}

inline const LegacyMaterialPassPropertyLayout *LegacyMaterialPass::GetPropertyLayout(const FastKeywordContext &keywordValues)
{
    return GetPropertyLayout(shaderTemplate_->GetKeywordSet().ExtractValue(keywordValues));
}

inline const RC<ShaderTemplate> &LegacyMaterialPass::GetShaderTemplate() const
{
    return shaderTemplate_;
}

inline const std::string &LegacyMaterial::GetName() const
{
    return name_;
}

inline Span<MaterialProperty> LegacyMaterial::GetProperties() const
{
    return propertyLayout_->GetProperties();
}

inline RC<LegacyMaterialPass> LegacyMaterial::GetPassByIndex(int index)
{
    return passes_[index];
}

inline RC<LegacyMaterialPass> LegacyMaterial::GetPassByTag(MaterialPassTag tag)
{
    const int index = GetPassIndexByTag(tag);
    if(index < 0)
    {
        throw Exception(fmt::format("Tag {} not found in material {}", tag, name_));
    }
    return GetPassByIndex(index);
}

// return -1 when not found
inline int LegacyMaterial::GetPassIndexByTag(MaterialPassTag tag) const
{
    auto it = tagToIndex_.find(tag);
    return it != tagToIndex_.end() ? it->second : -1;
}

inline const RC<MaterialPropertyHostLayout> &LegacyMaterial::GetPropertyLayout() const
{
    return propertyLayout_;
}

inline Span<RC<LegacyMaterialPass>> LegacyMaterial::GetPasses() const
{
    return passes_;
}

RTRC_END
