#pragma once

#include <Rtrc/Graphics/Material/ShaderTemplate.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Math/Vector3.h>
#include <Rtrc/Utility/ObjectCache.h>
#include <Rtrc/Utility/SharedObjectPool.h>
#include <Rtrc/Utility/SignalSlot.h>

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

// Property declared at material scope
struct MaterialProperty
{
    enum class Type
    {
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
        Buffer,
        Texture2D, // including Texture2D and Texture2DArray
        Sampler,
        AccelerationStructure,
    };

    static constexpr bool IsValue(Type type);
    bool IsValue() const;

    size_t GetValueSize() const;

    static const char *GetTypeName(Type type);
    const char *GetTypeName() const;

    Type type;
    std::string name;
};

// Describe how properties are stored in a material instance
// TODO optimize: using pooled string for property name
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
    size_t GetValueOffset(std::string_view name) const;

    // return -1 when not found
    int GetPropertyIndexByName(std::string_view name) const;
    const MaterialProperty &GetPropertyByName(std::string_view name) const;

private:

    // value0, value1, ..., resource0, resource1, ...
    std::vector<MaterialProperty> sortedProperties_;
    std::map<std::string, int, std::less<>> nameToIndex_;
    size_t valueBufferSize_;
    std::vector<size_t> valueIndexToOffset_;
};

// Describe how to create constantBuffer/bindingGroup for a material pass instance
class MaterialPassPropertyLayout
{
public:

    struct ValueReference
    {
        size_t offsetInValueBuffer;
        size_t offsetInConstantBuffer;
        size_t sizeInConstantBuffer;
    };

    struct ResourceReference
    {
        MaterialProperty::Type type;
        size_t indexInProperties;
        size_t indexInBindingGroup;
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
        RC<DynamicBuffer>     cbuffer) const;

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

    const std::set<std::string> &GetTags() const;

    KeywordSet::ValueMask ExtractKeywordValueMask(const KeywordValueContext &keywordValues) const;

    RC<Shader> GetShader(KeywordSet::ValueMask mask);
    RC<Shader> GetShader(const KeywordValueContext &keywordValues);
    RC<Shader> GetShader();

    const MaterialPassPropertyLayout *GetPropertyLayout(KeywordSet::ValueMask mask);
    const MaterialPassPropertyLayout *GetPropertyLayout(const KeywordValueContext &keywordValues);

    auto &GetShaderTemplate() const;

    template<typename...Args>
    Connection OnDestroy(Args&&...args) { return onDestroyCallbacks_.Connect(std::forward<Args>(args)...); }

private:

    friend class MaterialManager;

    std::set<std::string> tags_;
    RC<ShaderTemplate> shaderTemplate_;

    const MaterialPropertyHostLayout *parentPropertyLayout_ = nullptr;
    std::vector<Box<MaterialPassPropertyLayout>> materialPassPropertyLayouts_;

    tbb::spin_rw_mutex propertyLayoutsMutex_;

    Signal<SignalThreadPolicy::SpinLock> onDestroyCallbacks_;
};

class Material : public std::enable_shared_from_this<Material>, public InObjectCache
{
public:

    const std::string &GetName() const;
    Span<MaterialProperty> GetProperties() const;
    auto &GetPropertyLayout() const;

    // return -1 when not found
    int GetPassIndexByTag(std::string_view tag) const;
    RC<MaterialPass> GetPassByIndex(int index);
    RC<MaterialPass> GetPassByTag(std::string_view tag);
    Span<RC<MaterialPass>> GetPasses() const;

    RC<MaterialInstance> CreateInstance() const;

private:

    friend class MaterialManager;

    Device *device_ = nullptr;

    std::string name_;
    std::vector<RC<MaterialPass>> passes_;
    std::map<std::string, int, std::less<>> tagToIndex_;

    RC<MaterialPropertyHostLayout> propertyLayout_;
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

inline size_t MaterialPropertyHostLayout::GetValueOffset(std::string_view name) const
{
    auto it = nameToIndex_.find(name);
    if(it == nameToIndex_.end())
    {
        throw Exception(fmt::format("Property name not found: {}", name));
    }
    return GetValueOffset(it->second);
}

// return -1 when not found
inline int MaterialPropertyHostLayout::GetPropertyIndexByName(std::string_view name) const
{
    auto it = nameToIndex_.find(name);
    return it != nameToIndex_.end() ? it->second : -1;
}

inline const MaterialProperty &MaterialPropertyHostLayout::GetPropertyByName(std::string_view name) const
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

inline const std::set<std::string> &MaterialPass::GetTags() const
{
    return tags_;
}

inline KeywordSet::ValueMask MaterialPass::ExtractKeywordValueMask(const KeywordValueContext &keywordValues) const
{
    return shaderTemplate_->GetKeywordValueMask(keywordValues);
}

inline RC<Shader> MaterialPass::GetShader(KeywordSet::ValueMask mask)
{
    return shaderTemplate_->GetShader(mask);
}

inline RC<Shader> MaterialPass::GetShader(const KeywordValueContext &keywordValues)
{
    return GetShader(shaderTemplate_->GetKeywordValueMask(keywordValues));
}

inline RC<Shader> MaterialPass::GetShader()
{
    return shaderTemplate_->GetShader(0);
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

inline const MaterialPassPropertyLayout *MaterialPass::GetPropertyLayout(const KeywordValueContext &keywordValues)
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

inline RC<MaterialPass> Material::GetPassByTag(std::string_view tag)
{
    const int index = GetPassIndexByTag(tag);
    if(index < 0)
    {
        throw Exception(fmt::format("Tag {} not found in material {}", tag, name_));
    }
    return GetPassByIndex(index);
}

// return -1 when not found
inline int Material::GetPassIndexByTag(std::string_view tag) const
{
    auto it = tagToIndex_.find(tag);
    return it != tagToIndex_.end() ? it->second : -1;
}

inline auto &Material::GetPropertyLayout() const
{
    return propertyLayout_;
}

inline Span<RC<MaterialPass>> Material::GetPasses() const
{
    return passes_;
}

RTRC_END
