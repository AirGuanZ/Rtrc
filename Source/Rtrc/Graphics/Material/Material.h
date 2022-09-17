#pragma once

#include <Rtrc/Graphics/Material/ShaderTemplate.h>

RTRC_BEGIN

class ShaderTokenStream;

class SubMaterial : public Uncopyable
{
public:

    SubMaterial();

    uint32_t GetSubMaterialInstanceID() const;
    const std::set<std::string> &GetTags() const;
    RC<Shader> GetShader(const KeywordValueContext &keywordValues);

private:

    friend class MaterialManager;

    uint32_t subMaterialInstanceID_;
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

private:

    friend class MaterialManager;

    std::string name_;
    std::vector<RC<SubMaterial>> subMaterials_;
    std::map<std::string, int, std::less<>> tagToIndex_;
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
