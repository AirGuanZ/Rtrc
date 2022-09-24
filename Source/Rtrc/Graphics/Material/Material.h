#pragma once

#include <Rtrc/Graphics/Material/ShaderTemplate.h>
#include <Rtrc/Math/Vector3.h>

RTRC_BEGIN

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
    };

    bool IsValue() const
    {
        return static_cast<int>(type) < static_cast<int>(Type::Buffer);
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

    const char *GetValueTypeName() const
    {
        static const char *names[] =
        {
            "float", "float2", "float3", "float4",
            "uint",  "uint2",  "uint3",  "uint4",
            "int",   "int2",   "int3",   "int4",
        };
        assert(static_cast<int>(type) < GetArraySize(names));
        return names[static_cast<int>(type)];
    }

    Type type;
    std::string name;
};

class MaterialPropertyHostLayout
{
public:

    explicit MaterialPropertyHostLayout(std::vector<MaterialProperty> properties);

    Span<MaterialProperty> GetProperties() const { return sortedProperties_; }
    Span<MaterialProperty> GetValueProperties() const { return Span(sortedProperties_.data(), GetValuePropertyCount()); }

    size_t GetValueBufferSize() const { return valueBufferSize_; }
    size_t GetValuePropertyCount() const { return valueIndexToOffset_.size(); }
    size_t GetValueOffset(size_t index) const { return valueIndexToOffset_[index]; }
    size_t GetValueOffset(std::string_view name) const;

private:

    // value0, value1, ..., resource0, resource1, ...
    std::vector<MaterialProperty> sortedProperties_;
    std::map<std::string, int, std::less<>> nameToIndex_;
    size_t valueBufferSize_;
    std::vector<size_t> valueIndexToOffset_;
};

class SubMaterial : public Uncopyable
{
public:

    SubMaterial();

    uint32_t GetSubMaterialInstanceID() const;
    const std::set<std::string> &GetTags() const;
    RC<Shader> GetShader(const KeywordValueContext &keywordValues);

private:

    friend class MaterialManager;

    uint32_t subMaterialID_;
    std::set<std::string> tags_;
    KeywordSet::PartialValueMask overrideKeywords_;
    RC<ShaderTemplate> shaderTemplate_;
};

class Material : public Uncopyable
{
public:

    const std::string &GetName() const;
    RC<SubMaterial> GetSubMaterialByIndex(int index);
    RC<SubMaterial> GetSubMaterialByTag(std::string_view tag);
    int GetSubMaterialIndexByTag(std::string_view tag) const;
    Span<MaterialProperty> GetProperties() const;

private:

    friend class MaterialManager;

    std::string name_;
    std::vector<RC<SubMaterial>> subMaterials_;
    std::map<std::string, int, std::less<>> tagToIndex_;

    RC<MaterialPropertyHostLayout> propertyLayout_;
};

class MaterialManager : public Uncopyable
{
public:

    MaterialManager();

    void SetDevice(RHI::DevicePtr device);
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

    RHI::DevicePtr device_;
    ShaderCompiler shaderCompiler_;

    bool debug_ = RTRC_DEBUG;

    SharedObjectPool<std::string, Material, true> materialPool_;
    SharedObjectPool<std::string, ShaderTemplate, true> shaderPool_;

    std::vector<std::string> filenames_;
    std::map<std::string, FileReference, std::less<>> materialNameToFilename_;
    std::map<std::string, FileReference, std::less<>> shaderNameToFilename_;
};

RTRC_END
