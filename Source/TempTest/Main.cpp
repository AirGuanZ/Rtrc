#include <iostream>

#include <Core/Serialization/TextSerializer.h>
#include <ShaderCompiler/Compiler/Compiler.h>
#include <ShaderCompiler/Database/ShaderDatabase.h>

#include <ShaderCompiler/Database/RegisterAllPreprocessedShaders.h>

enum class S
{
    S1, S2
};

struct A
{
    int x = 0;
    int y = 1;
    int z = 2;

    std::vector<int> w = { 3, 4, 5 };

    std::string e = "h\"\\\"ello";

    S s = S::S2;

    std::optional<S> s2 = S::S1;

    RTRC_AUTO_SERIALIZE(x, y, z, w, e, s, s2);
};

int main()
{
    Rtrc::SC::CompilableShader shader;
    shader.keywords.push_back(Rtrc::SC::BoolShaderKeyword{ "ENABLE_INSTANCE" });
    shader.keywords.push_back(Rtrc::SC::EnumShaderKeyword
    {
        .name = "SHADOW_MODE",
        .values = { "DISABLED", "HARD", "SOFT" }
    });
    Rtrc::CheckTextSerializationAndDeserialization<true>(shader);

    Rtrc::SC::ShaderDatabase database;
    RegisterAllPreprocessedShaders(database);
}
