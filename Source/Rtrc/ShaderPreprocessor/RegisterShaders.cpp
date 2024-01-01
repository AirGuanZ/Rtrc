#include <Rtrc/Graphics/Shader/ShaderDatabase.h>

#include "RegisterAllPreprocessedShaders.inl"

RTRC_BEGIN

void RegisterAllPreprocessedShadersInShaderDatabase(ShaderDatabase &database)
{
    RegisterAllPreprocessedShaders(database);
}

RTRC_END
