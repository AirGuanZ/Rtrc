#pragma once

#include <map>
#include <string>
#include <vector>

#include <Rtrc/Core/Uncopyable.h>

struct IDxcUtils;

RTRC_BEGIN

class DXC : public Uncopyable
{
public:

    enum class Target
    {
        Vulkan_1_3_VS_6_6,
        Vulkan_1_3_FS_6_6,
        Vulkan_1_3_CS_6_6,
        Vulkan_1_3_RT_6_6,

        DirectX12_VS_6_6,
        DirectX12_FS_6_6,
        DirectX12_CS_6_6,
        DirectX12_RT_6_6,
    };

    struct ShaderInfo
    {
        std::string source;
        std::string sourceFilename;
        std::string entryPoint; // Ignored when target is Vulkan_1_3_RT_6_6 or DirectX12_RT_6_6

        std::vector<std::string>           includeDirs;
        std::map<std::string, std::string> macros;

        bool bindless = false; // Enable spv extension for bindless descriptors
        bool rayQuery = false;
        bool rayTracing = false;
    };

    DXC();

    ~DXC();

    // When preprocessOutput != nullptr, returned value will always be empty
    // reflectionData can be non-null only when target is DirectX12_*
    std::vector<unsigned char> Compile(
        const ShaderInfo       &shaderInfo,
        Target                  target,
        bool                    debugMode,
        std::string            *preprocessOutput,
        std::vector<std::byte> *reflectionData) const;

    IDxcUtils *GetDxcUtils() const;

private:

    static bool SupportRayQueryDebugInfo(Target target);

    struct Impl;

    Impl *impl_;
};

RTRC_END