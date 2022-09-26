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
        Float, Float2, Float3, Float4,
        UInt,  UInt2,  UInt3,  UInt4,
        Int,   Int2,   Int3,   Int4,
        Buffer,
        Texture2D, // including Texture2D and Texture2DArray
        Sampler
    };

    static constexpr bool IsValue(Type type)
    {
        return static_cast<int>(type) < static_cast<int>(Type::Buffer);
    }

    bool IsValue() const
    {
        return IsValue(type);
    }

    size_t GetValueSize() const
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

    static const char *GetTypeName(Type type)
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

    const char *GetTypeName() const
    {
        return GetTypeName(type);
    }

    Type type;
    std::string name;
};

// Describe how properties are stored in a material instance
// TODO optimize: using pooled string for property name
class MaterialPropertyHostLayout
{
public:

    explicit MaterialPropertyHostLayout(std::vector<MaterialProperty> properties);

    Span<MaterialProperty> GetProperties() const { return sortedProperties_; }
    Span<MaterialProperty> GetValueProperties() const { return Span(sortedProperties_.data(), GetValuePropertyCount()); }

    size_t GetResourcePropertyCount() const { return sortedProperties_.size() - GetValuePropertyCount(); }
    Span<MaterialProperty> GetResourceProperties() const
    {
        return Span(sortedProperties_.data() + GetValuePropertyCount(), GetResourcePropertyCount());
    }

    size_t GetValueBufferSize() const { return valueBufferSize_; }
    size_t GetValuePropertyCount() const { return valueIndexToOffset_.size(); }
    size_t GetValueOffset(int index) const { return valueIndexToOffset_[index]; }
    size_t GetValueOffset(std::string_view name) const;

    // return -1 when not found
    int GetPropertyIndexByName(std::string_view name) const
    {
        auto it = nameToIndex_.find(name);
        return it != nameToIndex_.end() ? it->second : -1;
    }

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

    int GetBindingGroupIndex() const { return bindingGroupIndex_; }

    size_t GetConstantBufferSize() const { return constantBufferSize_; }
    void FillConstantBufferContent(const void *valueBuffer, void *outputData) const;

    const RC<BindingGroupLayout> &GetBindingGroupLayout() const { return bindingGroupLayout_; }
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

    SubMaterial()
    {
        static std::atomic<uint32_t> nextInstanceID = 0;
        subMaterialID_ = nextInstanceID++;
    }

    uint32_t GetSubMaterialInstanceID() const { return subMaterialID_; }
    const std::set<std::string> &GetTags() const { return tags_; }

    KeywordSet::ValueMask ExtractKeywordValueMask(const KeywordValueContext &keywordValues) const
    {
        return shaderTemplate_->GetKeywordValueMask(keywordValues);
    }

    const KeywordSet::PartialValueMask &GetOverrideKeywordValueMask() const
    {
        return overrideKeywords_;
    }

    RC<Shader> GetShader(KeywordSet::ValueMask mask)
    {
        mask = overrideKeywords_.Apply(mask);
        return shaderTemplate_->GetShader(mask);
    }

    RC<Shader> GetShader(const KeywordValueContext &keywordValues)
    {
        return GetShader(shaderTemplate_->GetKeywordValueMask(keywordValues));
    }

    const SubMaterialPropertyLayout *GetPropertyLayout(KeywordSet::ValueMask mask)
    {
        mask = overrideKeywords_.Apply(mask);
        if(!subMaterialPropertyLayouts_[mask])
        {
            auto newLayout = MakeBox<SubMaterialPropertyLayout>(*parentPropertyLayout_, *GetShader(mask));
            subMaterialPropertyLayouts_[mask] = std::move(newLayout);
        }
        return subMaterialPropertyLayouts_[mask].get();
    }

    const SubMaterialPropertyLayout *GetPropertyLayout(const KeywordValueContext &keywordValues)
    {
        return GetPropertyLayout(shaderTemplate_->GetKeywordValueMask(keywordValues));
    }

    auto &GetShaderTemplate() const { return shaderTemplate_; }

private:

    friend class MaterialManager;

    uint32_t subMaterialID_;
    std::set<std::string> tags_;
    KeywordSet::PartialValueMask overrideKeywords_;
    RC<ShaderTemplate> shaderTemplate_;

    const MaterialPropertyHostLayout *parentPropertyLayout_ = nullptr;
    std::vector<Box<SubMaterialPropertyLayout>> subMaterialPropertyLayouts_;
};

class Material : public Uncopyable, public std::enable_shared_from_this<Material>
{
public:

    const std::string &GetName() const { return name_; }
    Span<MaterialProperty> GetProperties() const { return propertyLayout_->GetProperties(); }
    RC<SubMaterial> GetSubMaterialByIndex(int index) { return subMaterials_[index]; }

    RC<SubMaterial> GetSubMaterialByTag(std::string_view tag);

    // return -1 when not found
    int GetSubMaterialIndexByTag(std::string_view tag) const
    {
        auto it = tagToIndex_.find(tag);
        return it != tagToIndex_.end() ? it->second : -1;
    }

    auto &GetPropertyLayout() const { return propertyLayout_; }

    RC<MaterialInstance> CreateInstance() const;

    Span<RC<SubMaterial>> GetSubMaterials() const { return subMaterials_; }

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

RTRC_END
