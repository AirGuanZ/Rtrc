#pragma once

#include <Graphics/Shader/Compiler.h>

RTRC_BEGIN

class StandaloneShaderCompiler : public Uncopyable
{
public:

    void SetDevice(Ref<Device> device);

    RC<Shader> Compile(
        const std::string                        &source,
        const std::map<std::string, std::string> &macros,
        bool                                      debug) const;

private:
    
    ShaderCompiler compiler_;
    DXC            dxc_;
};

RTRC_END
