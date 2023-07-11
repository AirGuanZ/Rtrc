#pragma once

#include <Rtrc/Graphics/Resource/BuiltinResources.h>
#include <Rtrc/Graphics/Resource/MaterialManager.h>
#include <Rtrc/Graphics/Resource/MeshManager.h>
#include <Rtrc/Utility/SmartPointer/ObserverPtr.h>

RTRC_BEGIN

class ResourceManager : public Uncopyable
{
public:
    
    using MeshFlags = MeshManager::Flags;

    explicit ResourceManager(ObserverPtr<Device> device, bool debugMode = RTRC_DEBUG);
    
    void AddMaterialFiles(const std::set<std::filesystem::path> &filenames);
    void AddShaderIncludeDirectory(std::string_view dir);

    BuiltinResourceManager &GetBuiltinResources();

    RC<Material>       GetMaterial      (const std::string &name);
    RC<ShaderTemplate> GetShaderTemplate(const std::string &name);
    RC<Shader>         GetShader        (const std::string &name); // Valid when no keyword is defined in corresponding shader template
    RC<Mesh>           GetMesh          (std::string_view name, MeshFlags flags = MeshFlags::None);

    RC<MaterialInstance> CreateMaterialInstance(const std::string &name);
    
private:

    ObserverPtr<Device>    device_;
    BuiltinResourceManager builtinResourceManager_;
    MaterialManager        materialManager_;
    MeshManager            meshManager_;
};

RTRC_END
