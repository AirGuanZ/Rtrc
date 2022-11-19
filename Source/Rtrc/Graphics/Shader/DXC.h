#pragma once

#include <map>
#include <string>
#include <vector>

#include <Rtrc/Utility/Uncopyable.h>

RTRC_BEGIN

class DXC : public Uncopyable
{
public:

    enum class Target
    {
        Vulkan_1_3_VS_6_0,
        Vulkan_1_3_FS_6_0,
        Vulkan_1_3_CS_6_0
    };

    struct ShaderInfo
    {
        std::string source;
        std::string sourceFilename;
        std::string entryPoint;
        std::vector<std::string> includeDirs;
        std::map<std::string, std::string> macros;
    };

    DXC();

    ~DXC();

    // when preprocessOutput != nullptr, returned value will always be empty
    std::vector<unsigned char> Compile(
        const ShaderInfo     &shaderInfo,
        Target                target,
        bool                  debugMode,
        std::string          *preprocessOutput) const;

private:

    struct Impl;

    Impl *impl_;
};

RTRC_END
