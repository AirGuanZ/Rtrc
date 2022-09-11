#pragma once

#include <Rtrc/Graphics/RHI/RHI.h>

RTRC_BEGIN

struct ShaderIOVar
{
    std::string semantic;
    RHI::VertexAttributeType type;
    bool isBuiltin;
    int location;
};

struct ShaderReflection
{
    std::vector<ShaderIOVar> inputVariables;
};

void ReflectSPIRV(Span<unsigned char> code, const char *entryPoint, ShaderReflection &result);

RTRC_END
