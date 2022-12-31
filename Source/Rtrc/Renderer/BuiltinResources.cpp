#include <Rtrc/Graphics/Mesh/MeshLoader.h>
#include <Rtrc/Renderer/BuiltinResources.h>
#include <Rtrc/Renderer/Utility/FullscreenPrimitive.h>

RTRC_BEGIN

BuiltinResourceManager::BuiltinResourceManager(Device &device)
    : device_(device)
{
    materialManager_.SetDebugMode(RTRC_DEBUG);
    materialManager_.SetDevice(&device_);
    materialManager_.SetRootDirectory("./Asset/Builtin/Material/");
    
    LoadBuiltinTextures();
    LoadBuiltinMeshes();
    LoadBuiltinMaterials();

    fullscreenTriangle_ = GetFullscreenTriangle(device_);
    fullscreenQuad_ = GetFullscreenQuad(device_);
}

Device &BuiltinResourceManager::GetDevice() const
{
    return device_;
}

const RC<Mesh> &BuiltinResourceManager::GetBuiltinMesh(BuiltinMesh mesh) const
{
    return meshes_[EnumToInt(mesh)];
}

const RC<Texture> &BuiltinResourceManager::GetBuiltinTexture(BuiltinTexture texture) const
{
    return textures_[EnumToInt(texture)];
}

const RC<Material> &BuiltinResourceManager::GetBuiltinMaterial(BuiltinMaterial material) const
{
    return materials_[EnumToInt(material)];
}

const Mesh &BuiltinResourceManager::GetFullscreenTriangleMesh() const
{
    return fullscreenTriangle_;
}

const Mesh &BuiltinResourceManager::GetFullscreenQuadMesh() const
{
    return fullscreenQuad_;
}

void BuiltinResourceManager::LoadBuiltinTextures()
{
    textures_[EnumToInt(BuiltinTexture::Black2D)] = device_.CreateColorTexture2D(0, 0, 0, 255);
    textures_[EnumToInt(BuiltinTexture::White2D)] = device_.CreateColorTexture2D(255, 255, 255, 255);
}

void BuiltinResourceManager::LoadBuiltinMeshes()
{
    MeshLoader meshManager;
    meshManager.SetCopyContext(&device_.GetCopyContext());
    meshManager.SetRootDirectory("./Asset/Builtin/Mesh/");

#define LOAD_BUILTIN_MESH(NAME) \
    meshes_[EnumToInt(BuiltinMesh::NAME)] = ToRC(meshManager.LoadFromObjFile(#NAME ".obj"))
    LOAD_BUILTIN_MESH(Cube);
#undef LOAD_BUILTIN_MESH
}

void BuiltinResourceManager::LoadBuiltinMaterials()
{
#define LOAD_BUILTIN_MATERIAL(NAME) \
    materials_[EnumToInt(BuiltinMaterial::NAME)] = materialManager_.GetMaterial("Builtin/" #NAME)
    LOAD_BUILTIN_MATERIAL(DeferredLighting);
    LOAD_BUILTIN_MATERIAL(Atmosphere);
#undef LOAD_BUILTIN_MATERIAL
}

RTRC_END
