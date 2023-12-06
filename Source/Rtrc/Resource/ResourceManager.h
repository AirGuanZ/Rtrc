#pragma once

#include <Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/Resource/LegacyMaterialManager.h>
#include <Rtrc/Resource/MeshManager.h>
#include <Rtrc/Resource/ShaderManager.h>

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

    explicit ResourceManager(ObserverPtr<Device> device, bool debugMode = RTRC_DEBUG);

    ObserverPtr<Device> GetDevice() const { return device_; }
    
    ObserverPtr<LegacyMaterialManager> GetMaterialManager() { return materialManager_; }
    ObserverPtr<MeshManager>           GetMeshManager()     { return meshManager_;     }
    ObserverPtr<ShaderManager>         GetShaderManager()   { return shaderManager_; }
    
    // General resource loading

    RC<LegacyMaterial> GetMaterial      (const std::string &name);
    RC<ShaderTemplate> GetShaderTemplate(const std::string &name, bool persistent);
    RC<Shader>         GetShader        (const std::string &name, bool persistent); // Valid when no keyword is defined in corresponding shader template
    RC<Mesh>           GetMesh          (std::string_view name, MeshFlags flags = MeshFlags::None);

    RC<LegacyMaterialInstance> CreateMaterialInstance(const std::string &name);

    template<TemplateStringParameter Name> RC<ShaderTemplate> GetShaderTemplate();
    template<TemplateStringParameter Name> RC<Shader>         GetShader();

    // Builtin resources
    
    const RC<Mesh>    &GetBuiltinMesh   (BuiltinMesh     mesh)     const;
    const RC<Texture> &GetBuiltinTexture(BuiltinTexture  texture)  const;

private:

    void LoadBuiltinTextures();
    void LoadBuiltinMeshes();

    ObserverPtr<Device>   device_;
    LegacyMaterialManager materialManager_;
    MeshManager           meshManager_;
    ShaderManager         shaderManager_;
    
    std::array<RC<Texture>,  EnumCount<BuiltinTexture>> textures_;
    std::array<RC<Mesh>,     EnumCount<BuiltinMesh>>    meshes_;
};

template<TemplateStringParameter Name>
RC<ShaderTemplate> ResourceManager::GetShaderTemplate()
{
    return shaderManager_.GetShaderTemplate<Name>();
}

template<TemplateStringParameter Name>
RC<Shader> ResourceManager::GetShader()
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

RTRC_END
