#pragma once

#include <Rtrc/Resource/LocalCache/LocalMaterialCache.h>
#include <Rtrc/Resource/LocalCache/LocalShaderCache.h>
#include <Rtrc/Resource/Material/LegacyMaterial.h>
#include <Graphics/Shader/Compiler.h>
#include <Graphics/Shader/ShaderDatabase.h>

RTRC_BEGIN

class LegacyMaterialManager;

void RegisterAllPreprocessedMaterialsInMaterialManager(LegacyMaterialManager &manager);

class LegacyMaterialManager : public Uncopyable
{
public:

    LegacyMaterialManager();

    void SetDevice(ObserverPtr<Device> device);
    void SetDebug(bool debug);

    void AddMaterial(RawMaterialRecord rawMaterial);

    RC<LegacyMaterial> GetMaterial(std::string_view name);
    RC<LegacyMaterial> GetMaterial(GeneralPooledString name);

    RC<ShaderTemplate> GetShaderTemplate(std::string_view name, bool persistent);
    RC<Shader>         GetShader(std::string_view name, bool persistent);

    RC<LegacyMaterialInstance> CreateMaterialInstance(std::string_view name);
    RC<LegacyMaterialInstance> CreateMaterialInstance(GeneralPooledString name);

    LocalMaterialCache &GetLocalMaterialCache();
    LocalShaderCache   &GetLocalShaderCache();

    template<TemplateStringParameter ShaderName>   const RC<ShaderTemplate> &GetCachedShaderTemplate();
    template<TemplateStringParameter ShaderName>   RC<Shader>                GetCachedShader();
    template<TemplateStringParameter MaterialName> RC<LegacyMaterial>              GetCachedMaterial();

private:

    using MaterialRecord = RawMaterialRecord;
    using MaterialRecordMap = std::map<GeneralPooledString, RawMaterialRecord>;

    ObserverPtr<Device>                                    device_;
    ObjectCache<GeneralPooledString, LegacyMaterial, true, true> materialPool_;
    MaterialRecordMap                                      materialRecords_;
    ShaderDatabase                                         shaderDatabase_;

    Box<LocalShaderCache>   localShaderCache_;
    Box<LocalMaterialCache> localMaterialCache_;
};

template<TemplateStringParameter ShaderName>
const RC<ShaderTemplate> &LegacyMaterialManager::GetCachedShaderTemplate()
{
    static LocalCachedShaderStorage staticStorage;
    const LocalCachedShaderHandle handle{ this, &staticStorage, ShaderName.GetString()};
    return handle.Get();
}

template<TemplateStringParameter ShaderName>
RC<Shader> LegacyMaterialManager::GetCachedShader()
{
    return GetCachedShaderTemplate<ShaderName>()->GetVariant();
}

template<TemplateStringParameter MaterialName>
RC<LegacyMaterial> LegacyMaterialManager::GetCachedMaterial()
{
    static LocalCachedMaterialStorage _staticMaterialStorage;
    const LocalCachedMaterialHandle handle{ this, &_staticMaterialStorage, MaterialName.GetString() };
    return handle.Get();
}

RTRC_END
