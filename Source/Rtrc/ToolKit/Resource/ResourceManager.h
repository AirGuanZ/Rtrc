#pragma once

#include <Rtrc/Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/ToolKit/Resource/Mesh/MeshManager.h>
#include <Rtrc/ToolKit/Resource/Shader/ShaderManager.h>

RTRC_BEGIN

enum class BuiltinTexture
{
    Black2D,      // R8G8B8A8_UNorm, RGB = 0, A = 1
    White2D,      // R8G8B8A8_UNorm, RGB = 1, A = 1
    BlueNoise256, // R8_UNorm, 256 * 256
    Count
};

enum class BuiltinMesh
{
    Cube,
    Count
};

class ResourceManager : public Uncopyable
{
public:
    
    using MeshFlags = MeshManager::Flags;

    explicit ResourceManager(Ref<Device> device, bool debugMode = RTRC_DEBUG);

    Ref<Device> GetDevice() const { return device_; }
    
    Ref<MeshManager>           GetMeshManager()     { return meshManager_;     }
    Ref<ShaderManager>         GetShaderManager()   { return shaderManager_; }
    
    // General resource loading

    RC<ShaderTemplate> GetShaderTemplate(const std::string &name, bool persistent);
    RC<Shader>         GetShader        (const std::string &name, bool persistent); // Valid when no keyword is defined in corresponding shader template
    RC<Mesh>           GetMesh          (std::string_view name, MeshFlags flags = MeshFlags::None);

    template<TemplateStringParameter Name> RC<ShaderTemplate> GetStaticShaderTemplate();
    template<TemplateStringParameter Name> RC<Shader>         GetStaticShader();

    // Builtin resources
    
    const RC<Mesh>    &GetBuiltinMesh   (BuiltinMesh     mesh)     const;
    const RC<Texture> &GetBuiltinTexture(BuiltinTexture  texture)  const;

    const RC<Buffer> &GetPoissonDiskSamples2048() const;

private:

    void LoadBuiltinTextures();
    void LoadBuiltinMeshes();
    void GeneratePoissonDiskSamples();

    Ref<Device>   device_;
    MeshManager   meshManager_;
    ShaderManager shaderManager_;
    
    std::array<RC<Texture>,  EnumCount<BuiltinTexture>> textures_;
    std::array<RC<Mesh>,     EnumCount<BuiltinMesh>>    meshes_;

    RC<Buffer> poissonDiskSamples2048_;
};

template<TemplateStringParameter Name>
RC<ShaderTemplate> ResourceManager::GetStaticShaderTemplate()
{
    return shaderManager_.GetShaderTemplate<Name>();
}

template<TemplateStringParameter Name>
RC<Shader> ResourceManager::GetStaticShader()
{
    return shaderManager_.GetShader<Name>();
}

inline const RC<Mesh> &ResourceManager::GetBuiltinMesh(BuiltinMesh mesh) const
{
    return meshes_[std::to_underlying(mesh)];
}

inline const RC<Texture> &ResourceManager::GetBuiltinTexture(BuiltinTexture texture) const
{
    return textures_[std::to_underlying(texture)];
}

inline const RC<Buffer> &ResourceManager::GetPoissonDiskSamples2048() const
{
    return poissonDiskSamples2048_;
}

RTRC_END
