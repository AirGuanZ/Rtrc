#pragma once

#include <Rtrc/Graphics/RHI/RHI.h>

struct SpvReflectShaderModule;

RTRC_BEGIN

struct ShaderIOVar
{
    RHI::VertexSemantic      semantic;
    RHI::VertexAttributeType type;
    bool                     isBuiltin;
    int                      location;
};

class ShaderReflection
{
public:

    virtual ~ShaderReflection() = default;

    virtual std::vector<ShaderIOVar> GetInputVariables() const = 0;
    virtual Vector3i GetComputeShaderThreadGroupSize() const = 0;
    virtual std::vector<RHI::RawShaderEntry> GetEntries() const = 0;
    virtual bool IsBindingUsed(std::string_view name) const = 0;
};

RTRC_END
