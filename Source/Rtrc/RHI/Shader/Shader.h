#pragma once

#include <Rtrc/RHI/RHI.h>
#include <Rtrc/RHI/Shader/Type.h>
#include <Rtrc/Utils/TypeIndex.h>

RTRC_RHI_BEGIN

class Shader : public Uncopyable
{
    
};

class BindingGroupInstanceVisitor : public Uncopyable
{
public:

    explicit BindingGroupInstanceVisitor(TypeIndex typeIndex);

    virtual ~BindingGroupInstanceVisitor() = default;

    template<typename C>
    void ModifyInstance(const RC<BindingGroupInstance> &instance, const C &group);

    template<typename C, typename M, typename V>
    void ModifyInstanceMember(const RC<BindingGroupInstance> &instance, const M C::*memberPtr, const V &value);

protected:

    virtual void ModifyInstance(const RC<BindingGroupInstance> &instance, const void *bindingGroupStruct) const = 0;

    virtual void ModifyInstanceMember(const RC<BindingGroupInstance> &instance, int memberIndex, const void *value) = 0;

    TypeIndex typeIndex_;
};

class ShaderCompiler : public Uncopyable
{
public:

    void SetVertexShaderSource(std::string source);

    void SetFragmentShaderSource(std::string source);

    template<BindingGroupStruct BindingGroup>
    void AddBindingGroup();

    RC<Shader> Compile() const;
};

RTRC_RHI_END
