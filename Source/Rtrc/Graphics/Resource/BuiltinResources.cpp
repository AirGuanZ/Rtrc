#include <Rtrc/Graphics/Resource/BuiltinResources.h>
#include <Rtrc/Graphics/Resource/MeshManager.h>
#include <Rtrc/Renderer/Utility/FullscreenPrimitive.h>
#include <Rtrc/Utility/Filesystem/DirectoryFilter.h>

RTRC_BEGIN

BuiltinResourceManager::BuiltinResourceManager(Device &device)
    : device_(device)
{
    materialManager_.SetDebugMode(RTRC_DEBUG);
    materialManager_.SetDevice(&device_);
    materialManager_.AddFiles($rtrc_get_files("Asset/Builtin/Material/*.*"));
    materialManager_.AddIncludeDirectory("Asset/Builtin/Material");

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
    return meshes_[std::to_underlying(mesh)];
}

const RC<Texture> &BuiltinResourceManager::GetBuiltinTexture(BuiltinTexture texture) const
{
    return textures_[std::to_underlying(texture)];
}

const RC<Material> &BuiltinResourceManager::GetBuiltinMaterial(BuiltinMaterial material) const
{
    return materials_[std::to_underlying(material)];
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
    textures_[std::to_underlying(BuiltinTexture::Black2D)] = device_.CreateColorTexture2D(0, 0, 0, 255, "BuiltinBlack2D");
    textures_[std::to_underlying(BuiltinTexture::White2D)] = device_.CreateColorTexture2D(255, 255, 255, 255, "BuiltinWhite2D");
}

void BuiltinResourceManager::LoadBuiltinMeshes()
{
#define LOAD_BUILTIN_MESH(NAME) \
    meshes_[std::to_underlying(BuiltinMesh::NAME)] = ToRC(MeshManager::Load(&device_, "Asset/Builtin/Mesh/" #NAME ".obj", {}))
    LOAD_BUILTIN_MESH(Cube);
#undef LOAD_BUILTIN_MESH
}

void BuiltinResourceManager::LoadBuiltinMaterials()
{
#define LOAD_BUILTIN_MATERIAL(NAME) \
    materials_[std::to_underlying(BuiltinMaterial::NAME)] = materialManager_.GetMaterial("Builtin/" #NAME)
    LOAD_BUILTIN_MATERIAL(DeferredLighting);
    LOAD_BUILTIN_MATERIAL(Atmosphere);
#undef LOAD_BUILTIN_MATERIAL
}

RTRC_END
