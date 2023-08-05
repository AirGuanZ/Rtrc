#pragma once

#include <Rtrc/Graphics/Resource/MaterialManager.h>
#include <Rtrc/Graphics/Resource/MeshManager.h>
#include <Rtrc/Utility/SmartPointer/ObserverPtr.h>

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

enum class BuiltinMaterial
{
    DeferredLighting,
    Atmosphere,
    ShadowMask,
    PathTracing,
    KajiyaGI,
    Count
};

class ResourceManager : public Uncopyable
{
public:
    
    using MeshFlags = MeshManager::Flags;

    explicit ResourceManager(ObserverPtr<Device> device, bool debugMode = RTRC_DEBUG);

    ObserverPtr<Device> GetDevice() const { return device_; }
    
    void AddMaterialFiles(const std::set<std::filesystem::path> &filenames);
    void AddShaderIncludeDirectory(std::string_view dir);

    ObserverPtr<MaterialManager> GetMaterialManager() { return materialManager_; }
    ObserverPtr<MeshManager>     GetMeshManager()     { return meshManager_;     }
    
    // General resource loading

    RC<Material>       GetMaterial      (const std::string &name);
    RC<ShaderTemplate> GetShaderTemplate(const std::string &name);
    RC<Shader>         GetShader        (const std::string &name); // Valid when no keyword is defined in corresponding shader template
    RC<Mesh>           GetMesh          (std::string_view name, MeshFlags flags = MeshFlags::None);

    RC<MaterialInstance> CreateMaterialInstance(const std::string &name);

    // Builtin resources
    
    const RC<Mesh>     &GetBuiltinMesh    (BuiltinMesh     mesh)     const;
    const RC<Texture>  &GetBuiltinTexture (BuiltinTexture  texture)  const;
    const RC<Material> &GetBuiltinMaterial(BuiltinMaterial material) const;

private:

    void LoadBuiltinTextures();
    void LoadBuiltinMeshes();
    void LoadBuiltinMaterials();

    ObserverPtr<Device> device_;
    MaterialManager     materialManager_;
    MeshManager         meshManager_;
    
    std::array<RC<Texture>,  EnumCount<BuiltinTexture>>  textures_;
    std::array<RC<Mesh>,     EnumCount<BuiltinMesh>>     meshes_;
    std::array<RC<Material>, EnumCount<BuiltinMaterial>> materials_;
};

inline const RC<Mesh> &ResourceManager::GetBuiltinMesh(BuiltinMesh mesh) const
{
    return meshes_[std::to_underlying(mesh)];
}

inline const RC<Texture> &ResourceManager::GetBuiltinTexture(BuiltinTexture texture) const
{
    return textures_[std::to_underlying(texture)];
}

inline const RC<Material> &ResourceManager::GetBuiltinMaterial(BuiltinMaterial material) const
{
    return materials_[std::to_underlying(material)];
}

RTRC_END
