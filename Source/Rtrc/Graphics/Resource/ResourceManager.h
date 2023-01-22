#pragma once

#include <Rtrc/Graphics/Resource/BuiltinResources.h>
#include <Rtrc/Graphics/Resource/MaterialManager.h>
#include <Rtrc/Graphics/Resource/MeshManager.h>

RTRC_BEGIN

class ResourceManager : public Uncopyable
{
public:

    explicit ResourceManager(Device *device, bool debugMode = RTRC_DEBUG);

    // .material, .shader -> Material
    // .obj -> Mesh
    void AddFiles(const std::set<std::filesystem::path> &filenames);

    void AddMaterialFiles(const std::set<std::filesystem::path> &filenames);
    void AddMeshFiles(const std::set<std::filesystem::path> &filenames);

    void AddShaderIncludeDirectory(std::string_view dir);

    RC<Material>       GetMaterial      (const std::string &name);
    RC<ShaderTemplate> GetShaderTemplate(const std::string &name);
    RC<Shader>         GetShader        (const std::string &name); // Valid when no keyword is defined in corresponding shader template

    RC<MaterialInstance> CreateMaterialInstance(const std::string &name);

    RC<Mesh> GetMesh(const std::string &name, const MeshManager::Options &options = {});

    const BuiltinResourceManager &GetBuiltinResources() const;

private:

    BuiltinResourceManager builtinResourceManager_;
    MaterialManager        materialManager_;
    MeshManager            meshManager_;
};

RTRC_END
