#pragma once

#include <Rtrc/Graphics/Material/ShaderTemplate.h>
#include <Rtrc/Graphics/Resource/ConstantBuffer.h>
#include <Rtrc/Graphics/Resource/ResourceManager.h>
#include <Rtrc/Math/Vector3.h>

/* Material
    Material:
        PropertyLayout
        SubMaterial:
            ShaderTemplate
            PropertyReferenceLayout
    MaterialInstance:
        Material
        PropertySheet
        SubMaterialInstance:
            SubMaterial
            ConstantBuffer
            BindingGroup
*/

RTRC_BEGIN

class MaterialInstance;
class ShaderTokenStream;

// Property declared at material scope
// SubMaterial can reference these properties using 'IMPORT_RESOURCE(Name)' in 'PerMaterial' group
//                                               or 'IMPORT_PROPERTY(Name)' in constant buffer
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
        Sampler
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

// Describe how to create constantBuffer/bindingGroup for a submaterial instance
class SubMaterialPropertyLayout
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

    SubMaterialPropertyLayout(const MaterialPropertyHostLayout &materialPropertyLayout, const Shader &shader);

    int GetBindingGroupIndex() const;

    size_t GetConstantBufferSize() const;
    void FillConstantBufferContent(const void *valueBuffer, void *outputData) const;

    const RC<BindingGroupLayout> &GetBindingGroupLayout() const;
    void FillBindingGroup(
        BindingGroup                   &bindingGroup,
        Span<RHI::RHIObjectPtr>         materialResources,
        const PersistentConstantBuffer &cbuffer) const;

private:

    size_t                      constantBufferSize_;
    int                         constantBufferIndexInBindingGroup_;
    std::vector<ValueReference> valueReferences_;

    int                            bindingGroupIndex_;
    RC<BindingGroupLayout>         bindingGroupLayout_;
    std::vector<ResourceReference> resourceReferences_;
};

class SubMaterial : public Uncopyable
{
public:

    SubMaterial();

    uint32_t GetSubMaterialInstanceID() const;
    const std::set<std::string> &GetTags() const;

    KeywordSet::ValueMask ExtractKeywordValueMask(const KeywordValueContext &keywordValues) const;

    RC<Shader> GetShader(KeywordSet::ValueMask mask);
    RC<Shader> GetShader(const KeywordValueContext &keywordValues);

    const SubMaterialPropertyLayout *GetPropertyLayout(KeywordSet::ValueMask mask);
    const SubMaterialPropertyLayout *GetPropertyLayout(const KeywordValueContext &keywordValues);

    auto &GetShaderTemplate() const;

private:

    friend class MaterialManager;

    uint32_t subMaterialID_;
    std::set<std::string> tags_;
    RC<ShaderTemplate> shaderTemplate_;

    const MaterialPropertyHostLayout *parentPropertyLayout_ = nullptr;
    std::vector<Box<SubMaterialPropertyLayout>> subMaterialPropertyLayouts_;
};

class Material : public Uncopyable, public std::enable_shared_from_this<Material>
{
public:

    const std::string &GetName() const;
    Span<MaterialProperty> GetProperties() const;
    auto &GetPropertyLayout() const;

    // return -1 when not found
    int GetSubMaterialIndexByTag(std::string_view tag) const;
    RC<SubMaterial> GetSubMaterialByIndex(int index);
    RC<SubMaterial> GetSubMaterialByTag(std::string_view tag);
    Span<RC<SubMaterial>> GetSubMaterials() const;

    RC<MaterialInstance> CreateInstance() const;

private:

    friend class MaterialManager;

    ResourceManager *resourceManager_ = nullptr;

    std::string name_;
    std::vector<RC<SubMaterial>> subMaterials_;
    std::map<std::string, int, std::less<>> tagToIndex_;

    RC<MaterialPropertyHostLayout> propertyLayout_;
};

class MaterialManager : public Uncopyable
{
public:

    MaterialManager();

    void SetResourceManager(ResourceManager *resourceManager);
    void SetRootDirectory(std::string_view directory);
    void SetDebugMode(bool debug);

    RC<Material> GetMaterial(std::string_view name);
    RC<ShaderTemplate> GetShaderTemplate(std::string_view name);

    RC<BindingGroupLayout> GetBindingGroupLayout(const RHI::BindingGroupLayoutDesc &desc);

    void GC();

private:

    struct FileReference
    {
        int filenameIndex;
        size_t beginPos;
        size_t endPos;
    };

    void ProcessShaderFile(int filenameIndex, const std::string &filename);
    void ProcessMaterialFile(int filenameIndex, const std::string &filename);

    RC<Material> CreateMaterial(std::string_view name);
    RC<ShaderTemplate> CreateShaderTemplate(std::string_view name);

    RC<SubMaterial> ParsePass(ShaderTokenStream &tokens);
    MaterialProperty ParseProperty(MaterialProperty::Type propertyType, ShaderTokenStream &tokens);
    
    ResourceManager *resourceManager_;
    ShaderCompiler shaderCompiler_;

    bool debug_ = RTRC_DEBUG;

    SharedObjectPool<std::string, Material, true> materialPool_;
    SharedObjectPool<std::string, ShaderTemplate, true> shaderPool_;

    std::vector<std::string> filenames_;
    std::map<std::string, FileReference, std::less<>> materialNameToFilename_;
    std::map<std::string, FileReference, std::less<>> shaderNameToFilename_;
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
    return Span(sortedProperties_.data() + GetValuePropertyCount(), sortedProperties_.size() - GetValuePropertyCount());
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

inline int SubMaterialPropertyLayout::GetBindingGroupIndex() const
{
    return bindingGroupIndex_;
}

inline size_t SubMaterialPropertyLayout::GetConstantBufferSize() const
{
    return constantBufferSize_;
}

inline const RC<BindingGroupLayout> &SubMaterialPropertyLayout::GetBindingGroupLayout() const
{
    return bindingGroupLayout_;
}

inline SubMaterial::SubMaterial()
{
    static std::atomic<uint32_t> nextInstanceID = 0;
    subMaterialID_ = nextInstanceID++;
}

inline uint32_t SubMaterial::GetSubMaterialInstanceID() const
{
    return subMaterialID_;
}

inline const std::set<std::string> &SubMaterial::GetTags() const
{
    return tags_;
}

inline KeywordSet::ValueMask SubMaterial::ExtractKeywordValueMask(const KeywordValueContext &keywordValues) const
{
    return shaderTemplate_->GetKeywordValueMask(keywordValues);
}

inline RC<Shader> SubMaterial::GetShader(KeywordSet::ValueMask mask)
{
    return shaderTemplate_->GetShader(mask);
}

inline RC<Shader> SubMaterial::GetShader(const KeywordValueContext &keywordValues)
{
    return GetShader(shaderTemplate_->GetKeywordValueMask(keywordValues));
}

inline const SubMaterialPropertyLayout *SubMaterial::GetPropertyLayout(KeywordSet::ValueMask mask)
{
    if(!subMaterialPropertyLayouts_[mask])
    {
        auto newLayout = MakeBox<SubMaterialPropertyLayout>(*parentPropertyLayout_, *GetShader(mask));
        subMaterialPropertyLayouts_[mask] = std::move(newLayout);
    }
    return subMaterialPropertyLayouts_[mask].get();
}

inline const SubMaterialPropertyLayout *SubMaterial::GetPropertyLayout(const KeywordValueContext &keywordValues)
{
    return GetPropertyLayout(shaderTemplate_->GetKeywordValueMask(keywordValues));
}

inline auto &SubMaterial::GetShaderTemplate() const
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

inline RC<SubMaterial> Material::GetSubMaterialByIndex(int index)
{
    return subMaterials_[index];
}

inline RC<SubMaterial> Material::GetSubMaterialByTag(std::string_view tag)
{
    const int index = GetSubMaterialIndexByTag(tag);
    if(index < 0)
    {
        throw Exception(fmt::format("Tag {} not found in material {}", tag, name_));
    }
    return GetSubMaterialByIndex(index);
}

// return -1 when not found
inline int Material::GetSubMaterialIndexByTag(std::string_view tag) const
{
    auto it = tagToIndex_.find(tag);
    return it != tagToIndex_.end() ? it->second : -1;
}

inline auto &Material::GetPropertyLayout() const
{
    return propertyLayout_;
}

inline Span<RC<SubMaterial>> Material::GetSubMaterials() const
{
    return subMaterials_;
}

RTRC_END
