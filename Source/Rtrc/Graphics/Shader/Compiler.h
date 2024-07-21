#pragma once

#include <Rtrc/Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/Graphics/Shader/Shader.h>
#include <Rtrc/ShaderCommon/DXC/DXC.h>

RTRC_BEGIN

class Device;

using CompilableShader = ShaderPreprocessingInput;

// TODO: support rtrc_group_struct
class ShaderCompiler : public Uncopyable
{
public:
    
    void SetDevice(Ref<Device> device);
    
    RC<Shader> Compile(
        const CompilableShader &shader,
        bool                    debug = RTRC_DEBUG) const;

    DXC &GetDXC() { return dxc_; }

private:

    enum class CompileStage
    {
        VS,
        FS,
        CS,
        RT,
        TS,
        MS,
        WG
    };
    
    void DoCompilation(
        const DXC::ShaderInfo      &shaderInfo,
        CompileStage                stage,
        bool                        debug,
        std::vector<unsigned char> &outData,
        Box<ShaderReflection>      &outRefl) const;

    void EnsureAllUsedBindingsAreGrouped(
        const CompilableShader      &shader,
        const Box<ShaderReflection> &refl,
        std::string_view             stage) const;

    DXC         dxc_;
    Ref<Device> device_;
};

RTRC_END
