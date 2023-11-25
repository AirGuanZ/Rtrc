#include <Graphics/Shader/ShaderDatabase.h>
#include <Rtrc/Resource/LegacyMaterialManager.h>

#include "RegisterAllPreprocessedMaterials.inl"
#include "RegisterAllPreprocessedShaders.inl"

RTRC_BEGIN

void RegisterAllPreprocessedShadersInShaderDatabase(ShaderDatabase &database)
{
    RegisterAllPreprocessedShaders(database);
}

void RegisterAllPreprocessedMaterialsInMaterialManager(LegacyMaterialManager &manager)
{
    RegisterAllPreprocessedMaterials(manager);
}

RTRC_END
