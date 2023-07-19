#pragma once

#include <Rtrc/Graphics/Device/LocalCache/LocalMaterialCache.h>
#include <Rtrc/Graphics/Device/LocalCache/LocalShaderCache.h>
#include <Rtrc/Graphics/Material/Material.h>

RTRC_BEGIN

class MaterialManager : public Uncopyable
{
public:

    MaterialManager();

    void SetDevice(Device *device);
    void SetDebugMode(bool debug);

    void AddIncludeDirectory(std::string_view directory);
    void AddFiles(const std::set<std::filesystem::path> &filenames);

    RC<Material>       GetMaterial      (const std::string &name);
    RC<ShaderTemplate> GetShaderTemplate(const std::string &name);
    RC<Shader>         GetShader        (const std::string &name);

    RC<MaterialInstance> CreateMaterialInstance(const std::string &name);

    LocalMaterialCache &GetLocalMaterialCache() { return *localMaterialCache_; }
    LocalShaderCache   &GetLocalShaderCache()   { return *localShaderCache_; }

    template<TemplateStringParameter MaterialName> RC<ShaderTemplate> GetCachedShaderTemplate();
    template<TemplateStringParameter MaterialName> RC<Shader>         GetCachedShader();
    template<TemplateStringParameter MaterialName> RC<Material>       GetCachedMaterial();

private:

    struct FileReference
    {
        int filenameIndex;
        size_t beginPos;
        size_t endPos;
    };

    void ProcessShaderFile(int filenameIndex, const std::string &filename);
    void ProcessMaterialFile(int filenameIndex, const std::string &filename);

    RC<Material>       CreateMaterial(std::string_view name);
    RC<ShaderTemplate> CreateShaderTemplate(std::string_view name);

    RC<MaterialPass> ParsePass(ShaderTokenStream &tokens);
    MaterialProperty ParseProperty(MaterialProperty::Type propertyType, ShaderTokenStream &tokens);

    Device        *device_ = nullptr;
    ShaderCompiler shaderCompiler_;

    bool debug_ = RTRC_DEBUG;

    ObjectCache<std::string, Material, true, true>       materialPool_;
    ObjectCache<std::string, ShaderTemplate, true, true> shaderPool_;

    std::vector<std::string>                          filenames_;
    std::map<std::string, FileReference, std::less<>> materialNameToFilename_;
    std::map<std::string, FileReference, std::less<>> shaderNameToFilename_;

    Box<LocalMaterialCache> localMaterialCache_;
    Box<LocalShaderCache>   localShaderCache_;
};

template<TemplateStringParameter MaterialName>
RC<ShaderTemplate> MaterialManager::GetCachedShaderTemplate()
{
    static LocalCachedShaderStorage _staticShaderStorage;
    const LocalCachedShaderHandle handle{ this, &_staticShaderStorage, MaterialName.GetString() };
    return handle.Get();
}

template<TemplateStringParameter MaterialName>
RC<Shader> MaterialManager::GetCachedShader()
{
    return GetCachedShaderTemplate<MaterialName>()->GetShader();
}

template<TemplateStringParameter MaterialName>
RC<Material> MaterialManager::GetCachedMaterial()
{
    static LocalCachedMaterialStorage _staticMaterialStorage;
    const LocalCachedMaterialHandle handle{ this, &_staticMaterialStorage, MaterialName.GetString() };
    return handle.Get();
}

RTRC_END
