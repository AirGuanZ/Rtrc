#pragma once

#include <Rtrc/Graphics/Material/ShaderTemplate.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Core/Container/ObjectCache.h>
#include <Rtrc/Core/Container/SharedObjectPool.h>
#include <Rtrc/Core/SignalSlot.h>
#include <Rtrc/Core/StringPool.h>

/* Material
    Material:
        PropertyLayout
        MaterialPass:
            ShaderTemplate
            PropertyReferenceLayout
    MaterialInstance:
        Material
        PropertySheet
        MaterialPassInstance:
            MaterialPass
            ConstantBuffer
            TempBindingGroup
*/

RTRC_BEGIN

class MaterialInstance;
class ShaderTokenStream;

using MaterialPassTag = GeneralPooledString;
#define RTRC_MATERIAL_PASS_TAG(X) RTRC_GENERAL_POOLED_STRING(X)

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

    Type                 type;
    MaterialPropertyName name;
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
    size_t GetValueOffset(MaterialPropertyName name) const;

    // return -1 when not found
    int GetPropertyIndexByName(MaterialPropertyName name) const;
    const MaterialProperty &GetPropertyByName(MaterialPropertyName name) const;

private:

    // value0, value1, ..., resource0, resource1, ...
    std::vector<MaterialProperty>                    sortedProperties_;
    std::map<MaterialPropertyName, int, std::less<>> nameToIndex_;
    size_t                                           valueBufferSize_;
    std::vector<size_t>                              valueIndexToOffset_;
};

// Describe how to create constantBuffer/bindingGroup for a material pass instance
class MaterialPassPropertyLayout
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

    MaterialPassPropertyLayout(const MaterialPropertyHostLayout &materialPropertyLayout, const Shader &shader);

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

class MaterialPass : public Uncopyable, public WithUniqueObjectID
{
public:

    MaterialPass() = default;
    ~MaterialPass();

    const std::vector<std::string>     &GetTags() const;
    const std::vector<MaterialPassTag> &GetPooledTags() const;

    KeywordSet::ValueMask ExtractKeywordValueMask(const KeywordContext &keywordValues) const;

    RC<Shader> GetShader(KeywordSet::ValueMask mask = 0);
    RC<Shader> GetShader(const KeywordContext &keywordValues);

    const MaterialPassPropertyLayout *GetPropertyLayout(KeywordSet::ValueMask mask);
    const MaterialPassPropertyLayout *GetPropertyLayout(const KeywordContext &keywordValues);

    auto &GetShaderTemplate() const;

    template<typename...Args>
    Connection OnDestroy(Args&&...args) const { return onDestroyCallbacks_.Connect(std::forward<Args>(args)...); }

private:

    friend class MaterialManager;

    std::vector<std::string>     tags_;
    std::vector<MaterialPassTag> pooledTags_;

    RC<ShaderTemplate> shaderTemplate_;

    const MaterialPropertyHostLayout *parentPropertyLayout_ = nullptr;
    std::vector<Box<MaterialPassPropertyLayout>> materialPassPropertyLayouts_;

    tbb::spin_rw_mutex propertyLayoutsMutex_;

    mutable Signal<SignalThreadPolicy::SpinLock> onDestroyCallbacks_;
};

class Material : public std::enable_shared_from_this<Material>, public InObjectCache, public WithUniqueObjectID
{
public:

    enum class BuiltinPass
    {
        GBuffer,
        Count
    };

    static const char *BuiltinPassName[EnumCount<BuiltinPass>];
    static const MaterialPassTag BuiltinPooledPassName[EnumCount<BuiltinPass>];

    const std::string &GetName() const;

    Span<MaterialProperty>                GetProperties() const;
    const RC<MaterialPropertyHostLayout> &GetPropertyLayout() const;

    // return -1 when not found
    int              GetPassIndexByTag(MaterialPassTag tag) const;
    RC<MaterialPass> GetPassByIndex(int index);
    RC<MaterialPass> GetPassByTag(MaterialPassTag tag);

    int GetBuiltinPassIndex(BuiltinPass pass) const;

    Span<RC<MaterialPass>> GetPasses() const;

    RC<MaterialInstance> CreateInstance() const;

private:

    friend class MaterialManager;

    Device *device_ = nullptr;

    std::string                                 name_;
    std::vector<RC<MaterialPass>>               passes_;
    std::map<MaterialPassTag, int, std::less<>> tagToIndex_;
    RC<MaterialPropertyHostLayout>              propertyLayout_;

    std::array<int, EnumCount<BuiltinPass>> builtinPassIndices_;
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
    assert(static_cast<int>(type) < GetArraySize(sizes));
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
    assert(static_cast<int>(type) < GetArraySize(names));
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

inline size_t MaterialPropertyHostLayout::GetValueOffset(MaterialPropertyName name) const
{
    auto it = nameToIndex_.find(name);
    if(it == nameToIndex_.end())
    {
        throw Exception(fmt::format("Property name not found: {}", name));
    }
    return GetValueOffset(it->second);
}

// return -1 when not found
inline int MaterialPropertyHostLayout::GetPropertyIndexByName(MaterialPropertyName name) const
{
    auto it = nameToIndex_.find(name);
    return it != nameToIndex_.end() ? it->second : -1;
}

inline const MaterialProperty &MaterialPropertyHostLayout::GetPropertyByName(MaterialPropertyName name) const
{
    const int index = GetPropertyIndexByName(name);
    if(index < 0)
    {
        throw Exception(fmt::format("Unknown material property: {}", name));
    }
    return GetProperties()[index];
}

inline int MaterialPassPropertyLayout::GetBindingGroupIndex() const
{
    return bindingGroupIndex_;
}

inline size_t MaterialPassPropertyLayout::GetConstantBufferSize() const
{
    return constantBufferSize_;
}

inline const RC<BindingGroupLayout> &MaterialPassPropertyLayout::GetBindingGroupLayout() const
{
    return bindingGroupLayout_;
}

inline MaterialPass::~MaterialPass()
{
    onDestroyCallbacks_.Emit();
}

inline const std::vector<std::string> &MaterialPass::GetTags() const
{
    return tags_;
}

inline const std::vector<MaterialPassTag> &MaterialPass::GetPooledTags() const
{
    return pooledTags_;
}

inline KeywordSet::ValueMask MaterialPass::ExtractKeywordValueMask(const KeywordContext &keywordValues) const
{
    return shaderTemplate_->GetKeywordValueMask(keywordValues);
}

inline RC<Shader> MaterialPass::GetShader(KeywordSet::ValueMask mask)
{
    return shaderTemplate_->GetShader(mask);
}

inline RC<Shader> MaterialPass::GetShader(const KeywordContext &keywordValues)
{
    return GetShader(shaderTemplate_->GetKeywordValueMask(keywordValues));
}

inline const MaterialPassPropertyLayout *MaterialPass::GetPropertyLayout(KeywordSet::ValueMask mask)
{
    {
        std::shared_lock readLock(propertyLayoutsMutex_);
        if(materialPassPropertyLayouts_[mask])
        {
            return materialPassPropertyLayouts_[mask].get();
        }
    }

    std::unique_lock writeLock(propertyLayoutsMutex_);
    if(!materialPassPropertyLayouts_[mask])
    {
        auto newLayout = MakeBox<MaterialPassPropertyLayout>(*parentPropertyLayout_, *GetShader(mask));
        materialPassPropertyLayouts_[mask] = std::move(newLayout);
    }
    return materialPassPropertyLayouts_[mask].get();
}

inline const MaterialPassPropertyLayout *MaterialPass::GetPropertyLayout(const KeywordContext &keywordValues)
{
    return GetPropertyLayout(shaderTemplate_->GetKeywordValueMask(keywordValues));
}

inline auto &MaterialPass::GetShaderTemplate() const
{
    return shaderTemplate_;
}

inline const std::string &Material::GetName() const
{
    return name_;
}

inline Span<MaterialProperty> Material::GetProperties() const
{
    return propertyLayout_->GetProperties();
}

inline RC<MaterialPass> Material::GetPassByIndex(int index)
{
    return passes_[index];
}

inline RC<MaterialPass> Material::GetPassByTag(MaterialPassTag tag)
{
    const int index = GetPassIndexByTag(tag);
    if(index < 0)
    {
        throw Exception(fmt::format("Tag {} not found in material {}", tag, name_));
    }
    return GetPassByIndex(index);
}

// return -1 when not found
inline int Material::GetPassIndexByTag(MaterialPassTag tag) const
{
    auto it = tagToIndex_.find(tag);
    return it != tagToIndex_.end() ? it->second : -1;
}

inline const RC<MaterialPropertyHostLayout> &Material::GetPropertyLayout() const
{
    return propertyLayout_;
}

inline int Material::GetBuiltinPassIndex(BuiltinPass pass) const
{
    return builtinPassIndices_[std::to_underlying(pass)];
}

inline Span<RC<MaterialPass>> Material::GetPasses() const
{
    return passes_;
}

RTRC_END
