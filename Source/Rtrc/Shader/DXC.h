#pragma once

#include <map>
#include <string>
#include <vector>

#include <Rtrc/Utils/Uncopyable.h>

RTRC_BEGIN

class DXC : public Uncopyable
{
public:

    enum class Target
    {
        Vulkan_1_3_VS_6_0,
        Vulkan_1_3_PS_6_0
    };

    struct ShaderInfo
    {
        std::string source;
        std::string sourceFilename;
        std::string entryPoint;
        std::map<std::string, std::string> macros;
    };

    DXC();

    ~DXC();

    std::vector<unsigned char> Compile(
        const ShaderInfo                         &shaderInfo,
        Target                                    target,
        bool                                      debugMode,
        const std::map<std::string, std::string> &virtualFiles) const;

private:

    struct Impl;

    Impl *impl_;
};

RTRC_END
