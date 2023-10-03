#include <Graphics/Shader/ShaderDatabase.h>
#include <Rtrc/Resource/MaterialManager.h>

#include "RegisterAllPreprocessedMaterials.inl"
#include "RegisterAllPreprocessedShaders.inl"

RTRC_BEGIN

void RegisterAllPreprocessedShadersInShaderDatabase(ShaderDatabase &database)
{
    RegisterAllPreprocessedShaders(database);
}

void RegisterAllPreprocessedMaterialsInMaterialManager(MaterialManager &manager)
{
    RegisterAllPreprocessedMaterials(manager);
}

RTRC_END
