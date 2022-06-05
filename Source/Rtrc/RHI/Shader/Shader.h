#pragma once

#include <Rtrc/RHI/RHI.h>
#include <Rtrc/RHI/Shader/Type.h>
#include <Rtrc/Utils/TypeIndex.h>

RTRC_RHI_BEGIN

class Shader : public Uncopyable
{
public:

    RC<RawShader> GetVertexShader() const;

    RC<RawShader> GetFragmentShader() const;
};

class ShaderCompiler : public Uncopyable
{
public:

    void SetVertexShaderSource(std::string source, std::string entry);

    void SetFragmentShaderSource(std::string source, std::string entry);

    template<BindingGroupStruct BindingGroup>
    void AddBindingGroup();

    RC<Shader> Compile() const;

    RC<BindingLayout> GenerateBindingLayout() const;
};

RTRC_RHI_END
