#pragma once

#include <Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/Resource/MaterialManager.h>
#include <Rtrc/Resource/MeshManager.h>

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
    
    ObserverPtr<MaterialManager> GetMaterialManager() { return materialManager_; }
    ObserverPtr<MeshManager>     GetMeshManager()     { return meshManager_;     }
    
    // General resource loading

    RC<Material>       GetMaterial      (const std::string &name);
    RC<ShaderTemplate> GetShaderTemplate(const std::string &name, bool persistent);
    RC<Shader>         GetShader        (const std::string &name, bool persistent); // Valid when no keyword is defined in corresponding shader template
    RC<Mesh>           GetMesh          (std::string_view name, MeshFlags flags = MeshFlags::None);

    RC<MaterialInstance> CreateMaterialInstance(const std::string &name);

    // Builtin resources
    
    const RC<Mesh>    &GetBuiltinMesh    (BuiltinMesh     mesh)     const;
    const RC<Texture> &GetBuiltinTexture (BuiltinTexture  texture)  const;

private:

    void LoadBuiltinTextures();
    void LoadBuiltinMeshes();

    ObserverPtr<Device> device_;
    MaterialManager     materialManager_;
    MeshManager         meshManager_;
    
    std::array<RC<Texture>,  EnumCount<BuiltinTexture>> textures_;
    std::array<RC<Mesh>,     EnumCount<BuiltinMesh>>    meshes_;
};

inline const RC<Mesh> &ResourceManager::GetBuiltinMesh(BuiltinMesh mesh) const
{
    return meshes_[std::to_underlying(mesh)];
}

inline const RC<Texture> &ResourceManager::GetBuiltinTexture(BuiltinTexture texture) const
{
    return textures_[std::to_underlying(texture)];
}

RTRC_END