#include <Rtrc/Graphics/Mesh/MeshLoader.h>
#include <Rtrc/Renderer/BuiltinResources.h>

RTRC_BEGIN

BuiltinResourceManager::BuiltinResourceManager(Device &device)
    : device_(device)
{
    LoadBuiltinMaterials();
    LoadBuiltinTextures();
    LoadBuiltinMeshes();
}

const RC<Shader> &BuiltinResourceManager::GetBuiltinShader(BuiltinShader shader) const
{
    return shaders_[EnumToInt(shader)];
}

const RC<Mesh> &BuiltinResourceManager::GetBuiltinMesh(BuiltinMesh mesh) const
{
    return meshes_[EnumToInt(mesh)];
}

const RC<Texture> &BuiltinResourceManager::GetBuiltinTexture(BuiltinTexture texture) const
{
    return textures_[EnumToInt(texture)];
}

void BuiltinResourceManager::LoadBuiltinMaterials()
{
    materialManager_.SetDebugMode(RTRC_DEBUG);
    materialManager_.SetDevice(&device_);
    materialManager_.SetRootDirectory("./Asset/Builtin/Material/");

#define LOAD_BUILTIN_SHADER(NAME) \
    shaders_[EnumToInt(BuiltinShader::NAME)] = materialManager_.GetShader("Builtin/" #NAME)

    LOAD_BUILTIN_SHADER(DeferredLighting);

#undef LOAD_BUILTIN_SHADER
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

RTRC_END
