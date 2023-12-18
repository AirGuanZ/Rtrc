#pragma once

#include <Rtrc/Resource/LocalCache/LocalMaterialCache.h>
#include <Rtrc/Resource/Material/LegacyMaterial.h>
#include <Rtrc/Resource/ShaderManager.h>

RTRC_BEGIN

class LegacyMaterialManager;

void RegisterAllPreprocessedMaterialsInMaterialManager(LegacyMaterialManager &manager);

class LegacyMaterialManager : public Uncopyable
{
public:

    LegacyMaterialManager();

    void SetDevice(Ref<Device> device);
    void SetShaderManager(Ref<ShaderManager> shaderManager);

    void AddMaterial(RawMaterialRecord rawMaterial);

    RC<LegacyMaterial> GetMaterial(std::string_view name);
    RC<LegacyMaterial> GetMaterial(GeneralPooledString name);

    RC<LegacyMaterialInstance> CreateMaterialInstance(std::string_view name);
    RC<LegacyMaterialInstance> CreateMaterialInstance(GeneralPooledString name);

    LocalMaterialCache &GetLocalMaterialCache();

    template<TemplateStringParameter MaterialName> RC<LegacyMaterial> GetCachedMaterial();

private:

    using MaterialRecord = RawMaterialRecord;
    using MaterialRecordMap = std::map<GeneralPooledString, RawMaterialRecord>;

    Ref<Device>        device_;
    Ref<ShaderManager> shaderManager_;

    ObjectCache<GeneralPooledString, LegacyMaterial, true, true> materialPool_;
    MaterialRecordMap                                            materialRecords_;
    Box<LocalMaterialCache>                                      localMaterialCache_;
};

template<TemplateStringParameter MaterialName>
RC<LegacyMaterial> LegacyMaterialManager::GetCachedMaterial()
{
    static LocalCachedMaterialStorage _staticMaterialStorage;
    const LocalCachedMaterialHandle handle{ this, &_staticMaterialStorage, MaterialName.GetString() };
    return handle.Get();
}

RTRC_END
