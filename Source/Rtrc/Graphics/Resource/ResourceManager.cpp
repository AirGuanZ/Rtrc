#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Resource/ResourceManager.h>

RTRC_BEGIN

ResourceManager::ResourceManager(Device *device, bool debugMode)
    : device_(device), builtinResourceManager_(*device)
{
    materialManager_.SetDevice(device);
    materialManager_.SetDebugMode(debugMode);
    meshManager_.SetDevice(device);
}

void ResourceManager::AddMaterialFiles(const std::set<std::filesystem::path> &filenames)
{
    materialManager_.AddFiles(filenames);
}

void ResourceManager::AddShaderIncludeDirectory(std::string_view dir)
{
    materialManager_.AddIncludeDirectory(dir);
}

const BuiltinResourceManager &ResourceManager::GetBuiltinResources() const
{
    return builtinResourceManager_;
}

RC<Material> ResourceManager::GetMaterial(const std::string &name)
{
    return materialManager_.GetMaterial(name);
}

RC<ShaderTemplate> ResourceManager::GetShaderTemplate(const std::string &name)
{
    return materialManager_.GetShaderTemplate(name);
}

RC<Shader> ResourceManager::GetShader(const std::string &name)
{
    return materialManager_.GetShader(name);
}

RC<Mesh> ResourceManager::GetMesh(std::string_view name, const MeshManager::Options &options)
{
    return meshManager_.GetMesh(name, options);
}

RC<MaterialInstance> ResourceManager::CreateMaterialInstance(const std::string &name)
{
    return materialManager_.CreateMaterialInstance(name);
}

RTRC_END
